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
 * Description          : Generic URI of remote file
 *
 ***************************************************************************/

#include "fs_url.h"
#include "fs_mount.h"
#include "fs_samba.h"

#include <COMMON/BASIC/log.h>

/*****************************************************************************/
T_url::~T_url()
{
}

/*****************************************************************************/
T_mounted_url::T_mounted_url(N_Uri::T_uri uri,
                             const T_mounted_filesystem& mount)
  : _mount(mount), _full_path(uri.host() + uri.path())
{
  LOG(INFO, 8) << "Mounted URI full path: " << _full_path;
}

/*****************************************************************************/
T_mounted_url::T_mounted_url(const std::string& local_path,
                             const T_mounted_filesystem& mount)
  : _mount(mount), _local_path(local_path)
{
  LOG(INFO, 8) << "Mounted URI local path: " << _local_path;
}

/*****************************************************************************/
string
T_mounted_url::get_local_path() const
{
  if (_local_path.empty())
    {
      // Check that path contains remote root path
      string::size_type remote_path_pos = _full_path.find(_mount.path());
      if (remote_path_pos != string::npos)
        {
          _local_path = _mount.mount_point()
            + _full_path.substr(remote_path_pos + _mount.path().length());
          LOG(INFO, 8) << "Mounted URI local path: " << _local_path;
        }
      else
        {
          throw E_user("Invalid Mounted URI: full path does not contain remote path");
        }
    }
  return _local_path;
}

/*****************************************************************************/
N_Uri::T_uri
T_mounted_url::get_uri() const
{
  if (_full_path.empty())
    {
      // Check that local path contains mount point
      string::size_type mount_point_pos =_local_path.find(_mount.mount_point());
      if (mount_point_pos != string::npos)
        {
          _full_path = _mount.host() + _mount.path()
            + _local_path.substr(mount_point_pos + _mount.mount_point().length());
          LOG(INFO, 8) << "Mounted URI full path: " << _full_path;
        }
      else
        {
          throw E_user("Invalid Mounted URI: local path does not contain mount point");
        }
    }
  return N_Uri::T_uri("nfs://" + _full_path);
}

/*****************************************************************************/
T_samba_url::T_samba_url(N_Uri::T_uri uri)
  : _smb_url(uri.get_raw_uri(true, false))
{
}

T_samba_url::T_samba_url(const std::string& smb_url)
  : _smb_url(smb_url)
{
}

T_samba_url::T_samba_url(const std::string& path,
                         const T_samba_config& conf)
  : _smb_url("smb://" + conf.remote_host
           + "/" + conf.share_name
           + path)
{
}


string T_samba_url::get_local_path() const
{
  return _smb_url;
}

N_Uri::T_uri T_samba_url::get_uri() const
{
  return N_Uri::T_uri(_smb_url);
}


