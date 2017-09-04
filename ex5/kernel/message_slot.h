/*
 * message_slot.h
 *
 *  Created on: Jun 24, 2017
 *      Author: mike
 */

#ifndef MESSAGE_SLOT_H_
#define MESSAGE_SLOT_H_

#include <linux/ioctl.h>

#define ERROR	 							-1
#define SUCCESS 							0

/* The major device number - ioctls need to know it.*/
#define MAJOR_NUM 							245
/* Set the message of the device driver */
#define IOCTL_SET_MSG_CHNL					_IOW(MAJOR_NUM, 0, unsigned long)
#define BUF_SIZE					 		128
#define MAX_MSG						 		4
#define DEVICE_RANGE_NAME 					"message_slot_device" //TODO
#define DEVICE_FILE_NAME 					"message_slot_device"
#define DEVICE_PATH							"/dev/"DEVICE_FILE_NAME


#endif /* MESSAGE_SLOT_H_ */
