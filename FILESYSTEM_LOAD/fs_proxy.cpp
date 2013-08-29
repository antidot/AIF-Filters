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

/***************************************************************************
 *
 * (C) 2012 Antidot
 *
 * Description          : Generic interface for filesystem load
 *
 ***************************************************************************/

#include "fs_proxy.h"
#include <COMMON/BASIC/log.h>

using namespace N_Security;

T_filesystem_config::~T_filesystem_config()
{

}

T_filesystem_proxy::T_filesystem_proxy(T_filesystem_config_ptr conf)
  : _config(conf)
{
  if (_config->remote_host.empty())
    {
      LOG(ERROR, 1) << "Invalid remote filesystem configuration";
    }
}

T_filesystem_proxy::~T_filesystem_proxy()
{
}

/*****************************************************************************/
T_filesystem_acl::T_filesystem_acl(T_filesystem_proxy& proxy)
  : _fs(proxy)
{
}

T_filesystem_acl::~T_filesystem_acl()
{
}

/*****************************************************************************/
void T_filesystem_acl::add(const string& filepath,
                           const ACL& acl,
                           bool overwrite)
{
  super::add(filepath, acl, overwrite);

  if (*filepath.rbegin() != '/')
    {
      add(filepath + "/", acl);
    }
}

/*****************************************************************************/
ACL T_filesystem_acl::operator ()(const string& uri)
{
  try
    {
      return super::operator()(uri);
    }
  LOG_CATCH_AND_DISPLAY("No data in cache for "+uri, WARNING, 9)
  ACL acl = _fs.read_url_permissions(uri);
  add(uri, acl);
  return acl;
}