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
 * Description          : Generic URL of remote file
 *
 ***************************************************************************/
#ifndef _FILESYSTEM_URI_H
#define _FILESYSTEM_URI_H

#include <COMMON/URI/uri.h>
#include <COMMON/META/antidot.h>
#include <boost/shared_ptr.hpp>

class T_mounted_filesystem;
class T_samba_config;

/*****************************************************************************/
class T_url
{
public:
  virtual ~T_url();

  //! @brief Retrieves the local path used by proxy calls
  //! @return file or dir path (may contain a trailing slash or not)
  //! @exception E_user if uri does not match the nfs mount path
  virtual std::string get_local_path() const = 0;

  //! @brief Retrieves the uri
  virtual N_Uri::T_uri get_uri() const = 0;

};

typedef boost::shared_ptr<T_url> T_url_ptr;

/*****************************************************************************/
class T_mounted_url : public T_url
{
public:
  T_mounted_url(N_Uri::T_uri uri,
                const T_mounted_filesystem& mount);

  T_mounted_url(const std::string& local_path,
                const T_mounted_filesystem& mount);

  //! @brief Retrieves the local path ie /mount_path/remote_relative_path
  //! @return file or dir path (may contain a trailing slash or not)
  //! @exception E_user if uri does not match the nfs mount path
  std::string get_local_path() const;

  //! @brief Retrieves the NFS uri ie nfs://host/remote_absolute_path
  N_Uri::T_uri get_uri() const;

private:
  const T_mounted_filesystem&    _mount;
  mutable std::string   _full_path; // remote_host/mount_root/remote_path
  mutable std::string   _local_path;
};

/*****************************************************************************/
class T_samba_url : public T_url
{
public:
  T_samba_url(N_Uri::T_uri uri);
  T_samba_url(const std::string& smb_url);
  T_samba_url(const std::string& path, const T_samba_config& conf);

  //! @brief Retrieves the samba url ie smb:://host/share/path
  //! @return file or dir url
  std::string get_local_path() const;

  //! @brief Retrieves the samba uri
  N_Uri::T_uri get_uri() const;

private:
  mutable std::string _smb_url; // eg //host/share/path
  
};

#endif // _FILESYSTEM_URI_H
