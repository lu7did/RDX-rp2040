/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 * cli.h                                                                                       *
 * Simple command line based configuration tool                                                *
 *---------------------------------------------------------------------------------------------*
 *  Copyright  RDX project by Dr. Pedro E. Colla (LU7DZ) 2022. All rights reserved             *
 * This implementation provides a simplified command processor that can ge used to either      *
 * browse or modify configuration data stored in EEPROM, so the program configuration can be   *
 * changed without rebuild from sources                                                        *
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
 #ifdef CLITOOLS
/*--------------------------------------------------
 * Supporting structure for command processor
 */
struct  cmdSet {
  char  token[12];
  char  help[32];
  char  typearg;
  bool  toUpper;
  bool  toLower;
  int   min;
  int   max;
  CMD   handlerCmd;
  void* var;
};
#define MAXTOKEN     30
extern cmdSet langSet[MAXTOKEN];
#endif //CLITOOLS
