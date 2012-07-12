//  *******************************************************************
// 
//   Copyright (c) 2011  Evatronix SA 
//
//  *******************************************************************
//
//   Please review the terms of the license agreement before using     
//   this file. If you are not an authorized user, please destroy this 
//   source code file and notify Evatronix SA immediately that you     
//   inadvertently received an unauthorized copy.                      
//
//  *******************************************************************
/// @file           SDIO_Debug.h
/// @brief          File contains definitions of marcos which are used
///                 to debug SDIO software
/// @version        $Revision: 1.5 $
/// @author         Piotr Sroka
/// @date           $Date: 2011-02-01 13:21:50 $
//  *******************************************************************

#ifndef SDIO_DEBUG_H
#define SDIO_DEBUG_H
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "SDIO_Config.h"
#include "../environment/environment_config.h"


#if DEBUG_LEVEL == 0
#	define DEBUG_ERROR(message)
#	define DEBUG_INFO(message) 
#	define DEBUG_INTERRUPT(message)
#elif DEBUG_LEVEL == 1
#	define DEBUG_ERROR(message) { WRITE_STRING("#DEBUG-ERROR, "); DEBUG_MESSAGE(message); }
#	define DEBUG_INFO(message)
#	define DEBUG_INTERRUPT(message)
#elif DEBUG_LEVEL == 2
#	define DEBUG_ERROR(message) { WRITE_STRING("#DEBUG-ERROR, "); DEBUG_MESSAGE(message); }
#	define DEBUG_INFO(message) { WRITE_STRING("#DEBUG-INFO, "); DEBUG_MESSAGE(message); }
#	define DEBUG_INTERRUPT(message)
#elif DEBUG_LEVEL == 3
#	define DEBUG_ERROR(message) { WRITE_STRING("#DEBUG-ERROR, "); DEBUG_MESSAGE(message); }
#	define DEBUG_INFO(message) { WRITE_STRING("#DEBUG-INFO, "); DEBUG_MESSAGE(message); }
#	define DEBUG_INTERRUPT(message) { WRITE_STRING("#DEBUG-INTERRUPT, "); DEBUG_MESSAGE(message); }
#endif

#if DEBUG_LEVEL == 0
#	define SDIO_CHECK_IF_NULL( variable )
#	define SDIO_CHECK_IF_NOT_NULL( variable )
#	define SDIO_CHECK_IF_ZERO( variable )
#	define SDIO_CHECK_IF_NOT_ZERO( variable )
#else
#	define SDIO_CHECK_IF_NULL( variable )( assert( variable == NULL ) )
#	define SDIO_CHECK_IF_NOT_NULL( variable )( assert( variable != NULL ) )
#	define SDIO_CHECK_IF_ZERO( variable )( assert( variable == 0 ) )
#	define SDIO_CHECK_IF_NOT_ZERO( variable )( assert( variable != 0 ) )
#endif



/***************************************************************/
/*!  							   
 * @fn      void DebugMessage(const char *FuncName, 
 *                            const char *Format, 
 *                            ...)
 * @brief 	Function prints debug message
 * @param	FuncName Function name string 
 * @param   Format Charaters of format
 */
/***************************************************************/
void DebugMessage( const char *FuncName, const char *Format, ...);



#endif
