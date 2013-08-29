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
 * Description          : Generic interface for mounted filesystem
 *
 ***************************************************************************/
#ifndef _FILESYSTEM_MOUNT_H_
#define _FILESYSTEM_MOUNT_H_

#include "fs_proxy.h"

#include <AFS/SECURITY/unix_acl.h>
#include <PaF/API/filter.h>

typedef std::map<uint32_t, std::string> uid_gid_mapping_t;

/*****************************************************************************/
struct T_mount_config : public T_filesystem_config {
  std::string   remote_path;
  std::string   mount_point;
  std::string   mount_options;
  uid_gid_mapping_t users_mapping;
  uid_gid_mapping_t groups_mapping;
};

typedef boost::shared_ptr<T_mount_config> T_mount_config_ptr;

/*****************************************************************************/
//! @brief An implementation of filesystem proxy for mounted filesystem
//FIXME Currently only working with NFS!
class T_mounted_filesystem : public T_filesystem_proxy
{
public:
  T_mounted_filesystem(T_filesystem_config_ptr conf);
  virtual ~T_mounted_filesystem();

  const std::string& host() const { return _config->remote_host; }
  const std::string& path() const { return _config->remote_path; }
  const std::string& mount_point() const { return _config->mount_point; }
  const std::string& mount_options() const { return _config->mount_options; }
  const map<uint32_t, uint32_t> user_misses() const
  { return _on_mapping_miss.user_misses; }
  const map<uint32_t, uint32_t> group_misses() const
  { return _on_mapping_miss.group_misses; }

  virtual void connect();
  virtual void disconnect();

  virtual T_url_ptr create_url(const N_Uri::T_uri& uri) const;
  virtual T_url_ptr create_url(const std::string& fs_path) const;
  virtual bool check_if_dir_exists(const T_url& url);
  virtual bool check_if_file_exists(const T_url& url);
  virtual void get_directory_files(const T_url& url,
                                   std::set<std::string>& files);
  virtual void get_directory_subdirectories(const T_url& url,
                                            std::set<std::string>& subdirectories);
  virtual void read_file_content(const T_url& url,
                                 N_String::T_binary_string& data);
  virtual N_Security::ACL read_url_permissions(const T_url& url);
  virtual N_Security::ACL read_url_permissions(const string& localpath) ;
  virtual time_t read_file_mtime(const T_url& url);
  virtual time_t read_file_ctime(const T_url& url);

private:
  T_mount_config_ptr _config;
  N_Security::T_on_uid_mapping_miss _on_mapping_miss;
};

/*****************************************************************************/
class T_mount_acl: public T_filesystem_acl
{
public:
  T_mount_acl(T_mounted_filesystem&);
  virtual ~T_mount_acl();

  //! @brief Compute SAR layer
  virtual N_Security::SAR compute_sar_layer(const T_url& url);

  //! @brief Log the uuid/guid mapping errors
  virtual void log_mapping_errors(AFS::PaF::Handle& handle);

private:
  T_mounted_filesystem& _mount;

  void log_mapping_errors(AFS::PaF::Handle& handle,
                          const map<uint32_t, uint32_t>& errors,
                          std::string mapping_type);
};

#endif // _FILESYSTEM_MOUNT_H_
