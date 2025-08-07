//
// --------------------------------------------------------------------------
//  Gurux Ltd
//
//
//
// Filename:        $HeadURL$
//
// Version:         $Revision$,
//                  $Date$
//                  $Author$
//
// Copyright (c) Gurux Ltd
//
//---------------------------------------------------------------------------
//
//  DESCRIPTION
//
// This file is a part of Gurux Device Framework.
//
// Gurux Device Framework is Open Source software; you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; version 2 of the License.
// Gurux Device Framework is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// This code is licensed under the GNU General Public License v2.
// Full text may be retrieved at http://www.gnu.org/licenses/gpl-2.0.txt
//---------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
#include <assert.h>
#endif

#include "../include/gxmem.h"
#if _MSC_VER > 1400
#include <crtdbg.h>
#endif
#include "../include/gxset.h"
#include "../include/cosem.h"

int cosem_setValue(dlmsSettings* settings, gxValueEventArg* e)
{
    int ret = DLMS_ERROR_CODE_OK;
    if (e->index == 1)
    {

        // if (e->value.byteArr == NULL || e->value.byteArr->size - e->value.byteArr->position != 6)
        // {
        //     ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
        // }
        // else
        // {
        //     ret = bb_get(e->value.byteArr, e->target->logicalName, 6);
        // }
        // return ret;
    }
    switch (e->target->objectType)
    {
        case DLMS_OBJECT_TYPE_DATA:
            ret = cosem_setData(e);
        break;
        case DLMS_OBJECT_TYPE_REGISTER:
            ret = cosem_setRegister((gxRegister*)e->target, e->index, &e->value);
            break;
    
        default:
            ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}
