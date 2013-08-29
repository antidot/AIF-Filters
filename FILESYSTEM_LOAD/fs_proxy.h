/*
* Copyright 2013 Antidot opensource@antidot.net
https://github.com/antidot/AIF-Filters/
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

#ifndef _FILESYSTEM_PROXY_H
#define _FILESYSTEM_PROXY_H

#include "fs_url.h"

#include <AFS/SECURITY/tools.h>
#include <COMMON/STR/binstr.h>
#include <COMMON/META/antidot.h>
#include <COMMON/URI/uri.h>
#include <COMMON/URI/scheme.pb.h>
#include <PaF/API/filter.h>
#include <boost/shared_ptr.hpp>

/*****************************************************************************/
struct T_filesystem_config
{
public:
  virtual ~T_filesystem_config();
  N_Uri::Protocol fs_type;
  std::string remote_host;
};

typedef boost::shared_ptr<T_filesystem_config> T_filesystem_config_ptr;

/*****************************************************************************/
//! @brief An abstract interface for accessing a filesystem to load files
class T_filesystem_proxy
{
public:
  T_filesystem_proxy(T_filesystem_config_ptr conf);
  virtual ~T_filesystem_proxy();

  virtual void connect() = 0;
  virtual void disconnect() = 0;

  //! @brief Creates a URL from a given URI
  virtual T_url_ptr create_url(const N_Uri::T_uri& uri) const = 0;

  //! @brief Creates a URL from a filesystem path
  //! path is dependent on filesystem
  //!   - local path (NFS)
  //!   - full url (SAMBA)
  //! path is what is returned by get_directory_xxx() methods
  virtual T_url_ptr create_url(const std::string& fs_path) const = 0;

  //! @brief Returns true if the provided url is a directory
  //! @exception E_system if missing execute permission on parent dirs
  virtual bool check_if_dir_exists(const T_url& url) = 0;

  //! @brief Returns true if the provided url is an existing file
  //! @exception E_system if missing execute permission on parent dirs
  virtual bool check_if_file_exists(const T_url& url) = 0;

  //! @brief Retrieve the list of files within a directory
  //! @param url the url of directory to scan
  //! @param files (out) URLs of files in the directory
  //! @exception E_system if permission denied
  virtual void get_directory_files(const T_url& url,
                                   std::set<std::string>& files) = 0;

  //! @brief Retrieve the list of subdirectories of a directory
  //! @param url the url of directory to scan
  //! @param subdirectories (out) URLs of subdirectories
  //! @exception E_system if permission denied
  virtual void get_directory_subdirectories(const T_url& url,
                                            std::set<std::string>& subdirectories) = 0;

  //! @brief Read the content of a file
  virtual void read_file_content(const T_url& url,
                                 N_String::T_binary_string& data) = 0;

  //! @brief Read the permissions of a file or directory
  virtual N_Security::ACL read_url_permissions(const T_url& url) = 0;

  //! @brief Read the permissions of a file or directoty using localpath
  virtual N_Security::ACL read_url_permissions(const string& localpath) = 0;

  //! @brief Retrieve the last modification date of a file/dir
  virtual time_t read_file_mtime(const T_url& url) = 0;

  //! @brief Retrieve the last permissions change date of a file/dir
  virtual time_t read_file_ctime(const T_url& url) = 0;

private:
  T_filesystem_config_ptr _config;
};

/*****************************************************************************/
class T_filesystem_acl : public N_Security::T_cache_acl_builder
{
public:
  T_filesystem_acl(T_filesystem_proxy&);
  virtual ~T_filesystem_acl();

  //! @brief Add ACL entry into cache
  //! (actually two entries: one additional with a trailing slash)
  virtual void add(const string& filepath,
                   const N_Security::ACL& acl,
                   bool overwrite = false);

  //! @brief Compute ACL layer
  virtual N_Security::ACL operator()(const std::string& filepath);

  //! @brief Compute SAR layer
  virtual N_Security::SAR compute_sar_layer(const T_url& url) = 0;

  //! @brief Log the uuid/guid/sid mapping errors
  virtual void log_mapping_errors(AFS::PaF::Handle& handle) = 0;

private:
  T_filesystem_proxy& _fs;
  typedef N_Security::T_cache_acl_builder super;
};

#endif // _FILESYSTEM_PROXY_H
