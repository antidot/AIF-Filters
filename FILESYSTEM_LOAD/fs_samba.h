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
 * (C) 2013 Antidot
 *
 * Description          : SAMBA filesystem proxy
 *
 ***************************************************************************/
#ifndef _SAMBA_FILESYSTEM_H
#define _SAMBA_FILESYSTEM_H

#include "fs_proxy.h"
#include <AFS/SECURITY/win_acl.h>


/*****************************************************************************/
struct T_samba_config : public T_filesystem_config {
  std::string user;
  std::string password;
  std::string workgroup;
  std::string share_name;
  N_Security::T_sid_mapping sid_mapping;
  void add_sid_mappings(const std::map<std::string, std::string>& ,
                        N_Security::ActorType actor_type);
};

typedef boost::shared_ptr<T_samba_config> T_samba_config_ptr;

/*****************************************************************************/
class T_samba_filesystem : public T_filesystem_proxy
{
public:
  T_samba_filesystem(T_filesystem_config_ptr conf);
  virtual ~T_samba_filesystem();

  T_samba_config_ptr get_config() const;

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
  T_samba_config_ptr _config;
};

/*****************************************************************************/
class T_samba_acl : public T_filesystem_acl
{
  public:
    T_samba_acl(T_samba_filesystem&);
    virtual ~T_samba_acl();

    //! @brief Compute SAR layer
    virtual N_Security::SAR compute_sar_layer(const T_url& url);

    //! @brief Log the sid mapping errors
    virtual void log_mapping_errors(AFS::PaF::Handle& handle);

  private:
    typedef N_Security::T_cache_acl_builder super;
    T_samba_filesystem& _samba_fs;
};

#endif // _SAMBA_FILESYSTEM_H
