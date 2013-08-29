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
 * Description          : Generic proxy for mounted filesystem
 *
 ***************************************************************************/

#include "fs_mount.h"
#include "fs_url.h"
#include <COMMON/IO/io.h>
#include <COMMON/BASIC/log.h>
#include <sys/mount.h>
#include <sys/file.h>

using namespace N_Security;
using namespace boost;

/*****************************************************************************/
T_mounted_filesystem::T_mounted_filesystem(T_filesystem_config_ptr conf)
  : T_filesystem_proxy(conf),
    _config(dynamic_pointer_cast<T_mount_config>(conf))
{
  assert(_config.get());
  if (_config->remote_host.empty()
      || _config->remote_path.empty()
      || _config->mount_point.empty())
    {
      LOG(ERROR, 1) << "Invalid NFS mount configuration (check PaF configuration file)";
    }
}

/*****************************************************************************/
T_mounted_filesystem::~T_mounted_filesystem()
{
}

/*****************************************************************************/
T_url_ptr
T_mounted_filesystem::create_url(const N_Uri::T_uri& uri) const
{
  return T_url_ptr(new T_mounted_url(uri, *this));
}

/*****************************************************************************/
T_url_ptr
T_mounted_filesystem::create_url(const std::string& fs_path) const
{
  return T_url_ptr(new T_mounted_url(fs_path, *this));
}

/*****************************************************************************/
void T_mounted_filesystem::connect()
{
  LOG(INFO, 5) << "Mounting NFS filesystem : " << _config->remote_host
               << ":" << _config->remote_path ;
  if (!_config->mount_options.empty())
    {
      LOG(INFO, 5) << "With options: " << _config->mount_options;
    }
  try
    {
      string src = _config->remote_host + ":" + _config->remote_path;
      string dst = _config->mount_point;
      string opts = _config->mount_options.empty() ? ""
                      : " -o " + _config->mount_options + " ";
      string cmd = "sudo /bin/mount -r -t nfs " + opts + src + " " + dst;
      int32_t error_code(0);
      N_IO::system(cmd, &error_code);
      if (error_code != 0)
        {
          LOG(FATAL, 1) << "Could not mount filesystem! " << error_code;
        }
      LOG(INFO, 5) << "NFS mount done.";
    }
  catch(E_system& e)
    {
      LOG(FATAL, 1) << "Could not mount filesystem! " << e;
    }
}

/*****************************************************************************/
void T_mounted_filesystem::disconnect()
{
  LOG(INFO, 5) << "Unmounting NFS filesystem : " << _config->remote_host
               << ":" << _config->remote_path ;
  try
    {
      sleep(1); // avoid last file access interference
      string cmd = "sudo /bin/umount " + _config->mount_point;
      int32_t error_code(0);
      N_IO::system(cmd, &error_code);
      if (error_code != 0)
        {
          LOG(FATAL, 1) << "Could not umount filesystem! " << error_code;
        }
      LOG(INFO, 5) << "NFS umount done";
    }
  catch(E_system& e)
    {
      LOG(FATAL, 1) << "Could not umount filesystem! " << e;
    }
}

/*****************************************************************************/
bool T_mounted_filesystem::check_if_dir_exists(const T_url& uri)
{
  return N_IO::check_if_dir_exists(uri.get_local_path());
}

/*****************************************************************************/
bool T_mounted_filesystem::check_if_file_exists(const T_url& uri)
{
  return N_IO::check_if_file_exists(uri.get_local_path());
}

/*****************************************************************************/
void T_mounted_filesystem::get_directory_files(const T_url& url,
                                      set< string >& files)
{
  N_IO::get_directory_files(url.get_local_path(), files);
}

/*****************************************************************************/
void T_mounted_filesystem::get_directory_subdirectories(const T_url& url,
                                               set< string >& subdirectories)
{
  set<string> raw_subdirs;
  N_IO::get_directory_subdirectories(url.get_local_path(), raw_subdirs);
  BOOST_FOREACH(const string& subdir_name, raw_subdirs)
    {
      subdirectories.insert(url.get_local_path() + "/" + subdir_name);
    }
}

/*****************************************************************************/
void
T_mounted_filesystem::read_file_content(const T_url& uri, T_binary_string& data)
{
  string local_path = uri.get_local_path();
  LOG(INFO, 5) << "Reading content of " <<  local_path;
  data = N_IO::read_binary_file(local_path);
}

/*****************************************************************************/
ACL
T_mounted_filesystem::read_url_permissions(const T_url& uri)
{
  return read_url_permissions(uri.get_local_path());
}

/*****************************************************************************/
ACL
T_mounted_filesystem::read_url_permissions(const string& uri)
{
  LOG(INFO, 5) << "Reading permissions of " <<  uri;
  return build_acl_from_unix_path(uri,
                                  _config->users_mapping,
                                  _config->groups_mapping,
                                  _on_mapping_miss);
}

/*****************************************************************************/
time_t
T_mounted_filesystem::read_file_mtime(const T_url& uri)
{
  return N_IO::get_file_mtime(uri.get_local_path());
}

/*****************************************************************************/
time_t T_mounted_filesystem::read_file_ctime(const T_url& uri)
{
  return N_IO::get_file_ctime(uri.get_local_path());
}

/*****************************************************************************/
T_mount_acl::T_mount_acl(T_mounted_filesystem& mounted_fs)
 : T_filesystem_acl(mounted_fs), _mount(mounted_fs)
{
}

/*****************************************************************************/
T_mount_acl::~T_mount_acl()
{
}

/*****************************************************************************/
SAR T_mount_acl::compute_sar_layer(const T_url& uri)
{
  return build_sar_from_unix_acl(uri.get_local_path(),
                                 *this);
}

/*****************************************************************************/
void T_mount_acl::log_mapping_errors(AFS::PaF::Handle& handle)
{
  log_mapping_errors(handle, _mount.user_misses(), "USER");
  log_mapping_errors(handle, _mount.group_misses(), "GROUP");
}

/*****************************************************************************/
void T_mount_acl::log_mapping_errors(AFS::PaF::Handle& handle,
                                     const map< uint32_t, uint32_t >& errors,
                                     string mapping_type)
{
  if (!errors.empty())
    {
      ostringstream msg;
      msg << "Found " << errors.size() << " "
          << mapping_type << ((errors.size() > 1) ? "S" : "")
          << " without mapping";
      handle.log(N_Event::WARNING, msg.str());

      for (map<uint32_t, uint32_t>::const_iterator it = errors.begin();
           it != errors.end();
           ++it)
        {
          ostringstream msg;
          msg << "Could not find mapping for "
              << mapping_type << " with uid=" << it->first
              << " (" << it->second << " occurence(s))";
          handle.log(N_Event::WARNING, msg.str());
        }
    }
}
