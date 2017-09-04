/*
 * main.c
 *
 *  Created on: Jun 24, 2017
 *      Author: Mike Moldawsky
 */

#undef __KERNEL__
#define __KERNEL__ /* We're part of the kernel */
#undef MODULE
#define MODULE     /* Not a permanent part, though. */

#include "message_slot.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define MIN_PARAM							0
#define MAX_PARAM							3
#define NOT_SET								-1

#define PUT_USER_ERROR_STR   				"Failed to write data to user space buffer"
#define GET_USER_ERROR_STR   				"Failed to read data from user space buffer"
#define NOT_FOUND_ERROR_STR  				"Failed to find message slot %lu.\n"
#define IOCTL_PARAM_ERROR_STR				"Invalid parameter - channel index should be between [%d,%d] but <%d> given.\n"
#define IOCTL_OPERATION_ERROR_STR			"Invalid parameter - IOCTL operation.\n"
#define REG_ERROR_STR 						"Failed registering character device %d.\n"
#define MEMORY_ERROR_STR 					"Failed to allocate memory.\n"
#define REG_SUCCESS_STR 					"Registeration is a success. The major device number is %d.\n"
#define MKNOD_HELP_STR 						"If you want to talk to the device driver, you have to create a device file:\nmknod /dev/%s c %d 0\n"
#define RMNOD_HELP_STR      				"Dont forget to rm the device file and rmmod when you're done\n"
#define EXIT_MODULE_STR						"Removed module.\nCleaned all allocated memory:\t%d message_slots.\n"

#define PUT_USER_ERROR_MSG() 				printk(KERN_ALERT PUT_USER_ERROR_STR);
#define GET_USER_ERROR_MSG()	 			printk(KERN_ALERT GET_USER_ERROR_STR);
#define NOT_FOUND_ERROR_MSG(num) 			printk(KERN_ALERT NOT_FOUND_ERROR_STR, num);
#define IOCTL_PARAM_ERROR_MSG(param)		printk(KERN_ALERT IOCTL_PARAM_ERROR_STR, MIN_PARAM, MAX_PARAM, param);
#define IOCTL_OPERATION_ERROR_MSG()			printk(KERN_ALERT IOCTL_OPERATION_ERROR_STR);
#define REG_ERROR_MSG()				 		printk(KERN_ALERT REG_ERROR_STR, MAJOR_NUM);
#define MEMORY_ERROR_MSG()					printk(KERN_ALERT MEMORY_ERROR_STR);
#define REG_SUCCESS_MSG()	 				printk(REG_SUCCESS_STR, MAJOR_NUM);
#define MKNOD_HELP_MSG()	  				printk(KERN_INFO MKNOD_HELP_STR, DEVICE_FILE_NAME, MAJOR_NUM);
#define RMNOD_HELP_MSG() 					printk(KERN_INFO RMNOD_HELP_STR);
#define EXIT_MODULE_MSG(n)   				printk(EXIT_MODULE_STR, n);
#define INIT_SUCCESS_MSG()\
	REG_SUCCESS_MSG();\
	MKNOD_HELP_MSG();\
	RMNOD_HELP_MSG();

typedef struct message_slot{
	ino_t id;
    int cur_channel;
	char messages[MAX_MSG][BUF_SIZE];
} Message_slot;

typedef struct device_node{
	Message_slot msg_slot;
	struct device_node* next;
} Node;

typedef struct devices{
	Node dummy;
	Node* last;
} Devices;

static Node* find_message_slot(ino_t i_ino);

Node* find_message_slot(ino_t i_ino);
static Devices devices = {{{0}}};

/******************* Message Device Functions *********************/

static int device_open(struct inode *inode, struct file *file){
	Node *cur;
	if(find_message_slot(file->f_inode->i_ino) != NULL){
		return SUCCESS; /*device already has message slot element*/
	}
	/*device wasn't found*/
	if((cur = (Node*)kmalloc(sizeof(*cur), GFP_KERNEL)) == NULL){ /*messages buffers are not initialized if needed change to kzalloc */
		MEMORY_ERROR_MSG();
		return -ENOMEM;
	}
	/*Initializing cur node*/
	cur->next = NULL;
	/*Linked list pointers changes*/
	devices.last->next = cur;
	devices.last = cur;

	/*Message slot initialization*/
	cur->msg_slot.id = file->f_inode->i_ino;
	cur->msg_slot.cur_channel = NOT_SET;

	return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file){
	  return SUCCESS;
}

static ssize_t device_read(struct file *file, char __user * buffer, size_t length, loff_t * offset){
	int i, channel, rc;
	ssize_t total_read = 0;
	Node *cur;
	if((cur = find_message_slot(file->f_inode->i_ino)) == NULL){
		NOT_FOUND_ERROR_MSG(file->f_inode->i_ino);
		return ERROR;
	}
	channel = cur->msg_slot.cur_channel;

	if( channel == NOT_SET || channel < MIN_PARAM || channel > MAX_PARAM ){
		IOCTL_PARAM_ERROR_MSG(cur->msg_slot.cur_channel);
		return ERROR;
	}
	/*Writing message to user buffer*/
	for (i = 0; i < length && i < BUF_SIZE; i++){
	   if((rc = put_user(cur->msg_slot.messages[channel][i], buffer + i)) != SUCCESS){
		   PUT_USER_ERROR_MSG();
		   return ERROR;
	   }
	   total_read++;
	}

	cur->msg_slot.cur_channel = NOT_SET; /*reseting channel to not set*/
	return total_read;
}


static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset){
	int i, channel, rc;
	ssize_t total_write = 0;
	Node *cur;
	if((cur = find_message_slot(file->f_inode->i_ino)) == NULL){
		NOT_FOUND_ERROR_MSG(file->f_inode->i_ino);
		return ERROR;
	}
	channel = cur->msg_slot.cur_channel;

	if( channel == NOT_SET || channel < MIN_PARAM || channel > MAX_PARAM ){
		IOCTL_PARAM_ERROR_MSG(cur->msg_slot.cur_channel);
		return ERROR;
	}

	/*writing message to the channel buffer*/
	for (i = 0; i < length && i < BUF_SIZE; i++){
		if((rc = get_user(cur->msg_slot.messages[channel][i], buffer+i)) != SUCCESS){
		   GET_USER_ERROR_MSG();
		   return ERROR;
		}
		total_write++;
	}

	/*fill rest of channel buffer with zero*/
	while(i < BUF_SIZE)		cur->msg_slot.messages[channel][i++] = 0;

	cur->msg_slot.cur_channel = NOT_SET; /*reseting channel to not set*/
	return total_write;
}

static long device_ioctl(struct file* file, unsigned int ioctl_num, unsigned long ioctl_param){
	Node *cur;
	if (ioctl_num != IOCTL_SET_MSG_CHNL ){  /*Invalid ioctl operation*/
		IOCTL_OPERATION_ERROR_MSG();
		return -EINVAL;
	}

	if(ioctl_param < MIN_PARAM || ioctl_param > MAX_PARAM){ /*Invalid channel number*/
		IOCTL_PARAM_ERROR_MSG((int)ioctl_param);
		return -EINVAL;
	}

	if((cur = find_message_slot(file->f_inode->i_ino))== NULL){
		NOT_FOUND_ERROR_MSG(file->f_inode->i_ino);
		return ERROR;
	}

	cur->msg_slot.cur_channel = (int)ioctl_param;
	return SUCCESS;
}

static Node* find_message_slot(ino_t i_ino){
	Node *cur;
	cur = &devices.dummy;
	while((cur = cur->next) != NULL){
		if(cur->msg_slot.id == i_ino)/*device already has message slot element*/
			return cur;
	}
	return NULL; /*device wasn't found*/
}

/************************* Module Declarations **************************/

struct file_operations Fops = {
    .open = device_open,
    .read = device_read,
    .write = device_write,
    .release = device_release,
    .unlocked_ioctl= device_ioctl
};

static int __init simple_init(void){
    /* Register a character device.*/
    if (register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops) < 0) {
    	REG_ERROR_MSG();
    	return ERROR;
    }

    /*setting last linked list pointer to dummy node*/
    devices.last = &devices.dummy;

    /*Module loaded successfully*/
    INIT_SUCCESS_MSG();
    return SUCCESS;
}

static void __exit simple_cleanup(void){
	/*free module allocated memory*/
	Node *cur, *next;
	int removed = 0;
	cur = devices.dummy.next;
	while(cur != NULL){
		next = cur->next;
		kfree(cur);
		cur = next;
		removed++;
	}
	/*Unregister the device*/
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
    EXIT_MODULE_MSG(removed);
    return;
}

MODULE_LICENSE("GPL");
module_init(simple_init);
module_exit(simple_cleanup);

