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

#include "fs_samba.h"

#include <COMMON/BASIC/log.h>
#include <COMMON/STR/str.h>

#include "libsmbclient.h"
#include <errno.h>

using namespace N_Security;
using namespace boost;

namespace {
  // Unique Samba configuration for a filter instance
  // Use Samba client CONTEXT authentication to avoid global variable if needed
  static T_samba_config_ptr global_conf;

  static void
  get_auth_data_fn(const char * pServer,
                  const char * pShare,
                  char * pWorkgroup,
                  int maxLenWorkgroup,
                  char * pUsername,
                  int maxLenUsername,
                  char * pPassword,
                  int maxLenPassword)
  {
      const char *server = global_conf->remote_host.c_str();
      const char *share = global_conf->share_name.c_str();
      const char *username = global_conf->user.c_str();
      const char *password = global_conf->password.c_str();
      const char *workgroup = global_conf->workgroup.c_str();

      LOG(INFO, 8) << "Checking SAMBA authentication: " << pServer << "/" << pShare;
      if (strcmp(server, pServer) == 0 &&
          strcmp(share, pShare) == 0 &&
          *workgroup != '\0' &&
          *username != '\0')
        {
            strncpy(pWorkgroup, workgroup, maxLenWorkgroup - 1);
            strncpy(pUsername, username, maxLenUsername - 1);
            strncpy(pPassword, password, maxLenPassword - 1);
            return;
        }
  }
} // namespace

T_samba_filesystem::T_samba_filesystem(T_filesystem_config_ptr conf)
  : T_filesystem_proxy(conf),
    _config(dynamic_pointer_cast<T_samba_config>(conf))
{
  assert(_config.get());
  if (_config->share_name.empty()
      || _config->user.empty())
    {
      LOG(ERROR, 1) << "Invalid Samba configuration";
    }
  global_conf = _config;
}

T_samba_filesystem::~T_samba_filesystem()
{
}

T_samba_config_ptr
T_samba_filesystem::get_config() const
{
  return _config;
}

void T_samba_filesystem::connect()
{
  int err = smbc_init(get_auth_data_fn,  1);

  if (err < 0)
    {
      throw E_system("Cannot initialize Samba");
    }
}

void T_samba_filesystem::disconnect()
{
  // nothing to do
}

T_url_ptr T_samba_filesystem::create_url(const N_Uri::T_uri& uri) const
{
  return T_url_ptr(new T_samba_url(uri));
}

T_url_ptr T_samba_filesystem::create_url(const std::string& fs_path) const
{
  return T_url_ptr(new T_samba_url(fs_path));
}

bool T_samba_filesystem::check_if_dir_exists(const T_url& url)
{
  if (smbc_opendir(url.get_local_path().c_str()) < 0)
    {
      string errmsg (strerror(errno));
      LOG(INFO, 5) << "Internal samba error: " << errmsg;
      if ((errno == ENOENT) || (errno == ENOTDIR))
        {
          return false;
        }
      throw E_system("Could not check directory: " + errmsg);
    }
  return true;
}

bool T_samba_filesystem::check_if_file_exists(const T_url& url)
{
  struct stat file_info;
  if (smbc_stat(url.get_local_path().c_str(), &file_info) == 0)
    {
      bool res = S_ISREG(file_info.st_mode) ;
      return res;
    }
  string errmsg (strerror(errno));
  LOG(INFO, 5) << "Internal samba error: " << errmsg;
  return false;
}

void
T_samba_filesystem::get_directory_files(const T_url& url,
                                        set< std::string >& files)
{
  int dir_handle = smbc_opendir(url.get_local_path().c_str());
  if (dir_handle < 1)
    {
      string errmsg (strerror(errno));
      throw E_system("Could not open directory: " + errmsg);
    }

  struct smbc_dirent * entry;

  while ((entry = smbc_readdir(dir_handle)) != NULL)
    {
      uint32_t entry_type = entry->smbc_type;
      if (entry_type == SMBC_FILE)
        {
          string entry_name(entry->name);
          string entry_url(url.get_local_path() + "/"+ entry_name);
          files.insert(entry_url);
        }
    }
}

void
T_samba_filesystem::get_directory_subdirectories(const T_url& url,
                                                 set< std::string >& subdirectories)
{
  int dir_handle = smbc_opendir(url.get_local_path().c_str());
  if (dir_handle < 1)
    {
      string errmsg (strerror(errno));
      throw E_system("Could not open directory: " + errmsg);
    }

  struct smbc_dirent * entry;

  while ((entry = smbc_readdir(dir_handle)) != NULL)
    {
      uint32_t entry_type = entry->smbc_type;
      if (entry_type == SMBC_DIR)
        {
          string entry_name(entry->name);
          if ((entry_name != ".") && (entry_name != ".."))
            {
              string entry_url(url.get_local_path() + "/"+ entry_name);
              subdirectories.insert(entry_url);
            }
        }
    }

  smbc_closedir(dir_handle);
}

void
T_samba_filesystem::read_file_content(const T_url& url,
                                      T_binary_string& data)
{
  int fd = smbc_open(url.get_local_path().c_str(), O_RDONLY, 0666);
  if (fd < 0)
    {
      string errmsg (strerror(errno));
      throw E_system("Could not open file: " + errmsg);
    }

  struct stat file_info;
  int err = smbc_fstat(fd, &file_info);
  if (err < 0)
    {
      smbc_close(fd);
      string errmsg (strerror(errno));
      throw E_system("Could not stat file: " + errmsg);
    }

  size_t contents_len = file_info.st_size;
  char *contents = new char[contents_len];
  T_auto_delete_char dcontents(contents);
  size_t nb_read = smbc_read(fd, contents, contents_len);

  if (nb_read != contents_len)
    {
      stringstream msg_s;
      msg_s << "Short read: read only " << nb_read << " bytes ("
            << contents_len << " expected)" << endl;
      smbc_close(fd);
      throw E_system(msg_s);
    }

  N_String::T_binary_string contents_s(contents, contents_len);
  data.swap(contents_s);

  smbc_close(fd);
}

N_Security::ACL
T_samba_filesystem::read_url_permissions(const T_url& url)
{
  return read_url_permissions(url.get_local_path());
}

N_Security::ACL
T_samba_filesystem::read_url_permissions(const std::string& localpath)
{
  N_Security::ACL acl;
  static const char* attr_name = "system.nt_sec_desc.*";

  size_t contents_len(1024);
  char *contents = new char[contents_len];
  T_auto_delete_char dcontents(contents);

  LOG(INFO, 3) << "Reading permissions for: " << localpath;

  if (smbc_getxattr(localpath.c_str(), attr_name, contents, contents_len) < 0)
    {
      string errmsg (strerror(errno));
      throw E_system("Could not get ACL: " + errmsg);
    }

  return build_acl_from_nt_sec_desc(contents, _config->sid_mapping);
}

time_t
T_samba_filesystem::read_file_mtime(const T_url& url)
{
  struct stat file_info;
  int err = smbc_stat(url.get_local_path().c_str(), &file_info);
  if (err < 0)
    {
      string errmsg (strerror(errno));
      throw E_system("Could not stat file: " + errmsg);
    }
  return file_info.st_mtime;
}

time_t
T_samba_filesystem::read_file_ctime(const T_url& url)
{
  struct stat file_info;
  int err = smbc_stat(url.get_local_path().c_str(), &file_info);
  if (err < 0)
    {
      string errmsg (strerror(errno));
      throw E_system("Could not stat file: " + errmsg);
    }
  return file_info.st_ctime;
}

/*****************************************************************************/
T_samba_acl::T_samba_acl(T_samba_filesystem& samba_fs)
  : T_filesystem_acl(samba_fs), _samba_fs(samba_fs)
{
}

T_samba_acl::~T_samba_acl()
{
}

SAR
T_samba_acl::compute_sar_layer(const T_url& url)
{
  return build_sar_from_nt_acl(operator()(url.get_local_path()));
}

void
T_samba_acl::log_mapping_errors(AFS::PaF::Handle& handle)
{
  const map< string, uint32_t >&  errors
    = _samba_fs.get_config()->sid_mapping.get_misses();
  if (not errors.empty())
    {
      ostringstream msg;
      msg << "Found " << errors.size() << " SID"
          << ((errors.size() > 1) ? "s" : "")
          << " without mapping";
      handle.log(N_Event::WARNING, msg.str());

      for (map<string, uint32_t>::const_iterator it = errors.begin();
           it != errors.end();
           ++it)
        {
          ostringstream msg;
          msg << "Could not find mapping for user or group "
              << " with SID='" << it->first
              << "' (" << it->second << " occurence(s))";
          handle.log(N_Event::WARNING, msg.str());
        }
    }
}

/*****************************************************************************/
void
T_samba_config::add_sid_mappings(const map<string,string >& sid_map,
                                 ActorType actor_type)
{
  for (map<string,string>::const_iterator it = sid_map.begin();
       it != sid_map.end();
       ++it)
    {
      sid_mapping.add_actor(it->second, it->first, actor_type);
    }
}


