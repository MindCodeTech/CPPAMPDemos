/*----------------------------------------------------------------------------
 * Copyright © Microsoft Corporation. All rights reserved.
 *---------------------------------------------------------------------------*/

inline int abs(int x) restrict(amp)
{
    return (x<0)?-x:x;
}