/*
* Copyright 2013 Antidot opensource@antidot.net
* https://github.com/antidot/afs_filesystem_load
*
* afs_filesystem_load is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* afs_filesystem_load is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "fs_load.h"
#include <COMMON/BASIC/log.h>

#include <PaF/API/PIPE/pipe.h>

/*****************************************************************************/
int main(int argc, char *argv[])
{
  {
    START_FILTER(T_filesystem_load, argc, argv);
  }
}

//
// End of file
//
