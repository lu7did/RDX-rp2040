/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 * cat.h                                                                                       *
 * Simple CAT Processor                                                                        *
 *---------------------------------------------------------------------------------------------*
 *  Copyright  RDX project by Dr. Pedro E. Colla (LU7DZ) 2022. All rights reserved             *
 * This implementation provides a simplified CAT processor emulating the TS2000 protocol       *
 *---------------------------------------------------------------------------------------------*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
#ifdef CAT

/*-------------------------------------------------------------------------------------------
  CAT memory areas
*/

#define CATCMD_SIZE          64

int  cat_stat=0;
int  CAT_mode=2;
char buf[CATCMD_SIZE];
char CATResp[CATCMD_SIZE];
char CATCmd[CATCMD_SIZE];
char serialBuf[CATCMD_SIZE*2];
char resp[CATCMD_SIZE*2];
bool ignoreFA=false;

int  CATT1=0;
int  CATT2=0;
bool flipLED=false;

#endif //CAT
