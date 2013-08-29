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
 * Description          :   FILESYSTEM load filter
 *
 ****************************************************************************/

#include "fs_load.h"
#include "fs_mount.h"
#include "fs_samba.h"

#include <PaF/API/PIPE/pipe.h>

#include <AFS/PROTOBUF/serialize.h>
#include <AFS/MESSAGES/PaF/pipesfilters.pb.h>
#include <CONF/AFS7/conf.h>
#include <COMMON/BASIC/log.h>
#include <COMMON/META/antidot.h>

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <sys/file.h>
#include <fnmatch.h>

using namespace N_Security;
using namespace boost;


/*****************************************************************************/
void T_filesystem_load::log_stats()
{
  {
    ostringstream msg;
    msg << "Found " << _stats._nb_new_files << " new file(s)"
        << " in " << _stats._nb_directories << " director"
        << ((_stats._nb_directories > 1) ? "ies" : "y");
    _handle.log(N_Event::INFO, msg.str());
  }
  if (_stats._nb_deleted_files > 0)
    {
      ostringstream msg;
      msg << "Found " << _stats._nb_deleted_files << " deleted file(s)";
      _handle.log(N_Event::INFO, msg.str());
    }
}

/*****************************************************************************/
T_filesystem_load::T_filesystem_load(AFS::PaF::Configuration& configuration, 
                       AFS::PaF::Handle& handle)
  : ProcessorFilter(configuration, handle),
    _fs_type(N_Uri::NFS),
    _output_type(N_PaF::N_Layer::CONTENTS),
    _skip_non_readable_files(true),
    _stats()
{
  LOG(INFO, 9) << "T_filesystem_load::T_filesystem_load()";
}

/*****************************************************************************/
T_filesystem_load::~T_filesystem_load()
{
  LOG(INFO, 9) << "T_filesystem_load::~T_filesystem_load()";
  if (_fs_proxy.get())
  {
    _handle.log(N_Event::INFO, "Disconnecting from filesystem...");
    _fs_proxy->disconnect();
    _handle.log(N_Event::INFO, "OK - Disconnected.");
  }
  if (_acl_provider.get())
  {
    _acl_provider->log_mapping_errors(_handle);
  }

  log_stats();
}

/*****************************************************************************/
void T_filesystem_load::init()
{
  LOG(INFO, 9) << "T_nfs_load::init()";

  static const string skip_non_readable_files_arg_name("skip_non_readable_files");

  // Output layer
  _output_type = _configuration.get_output_type(N_PaF::N_Layer::CONTENTS);

  // Protocol
  {
    string protocol_str = _configuration.get_string("protocol");
    to_upper(protocol_str);
    if (not N_Uri::Protocol_Parse(protocol_str, &_fs_type))
      {
        _handle.log(N_Event::FATAL,
                    "Filesystem protocol: '" + protocol_str + "' invalid value");
      }
  }

  // Option for skipping unreadable files (if false => set doc KO)
  if (_configuration.has_arg(skip_non_readable_files_arg_name))
    {
      _skip_non_readable_files =
        _configuration.get_boolean(skip_non_readable_files_arg_name);
    }
  _handle.log(N_Event::INFO, "Filter argument: " + skip_non_readable_files_arg_name
               + " = " + to_string(_skip_non_readable_files));

  // Secured mode
  if (AFS::PaF::Pipe::pipe().is_secured())
    {
      _handle.log(N_Event::INFO, "Filter is running in SECURED mode");
    }

  create_filesystem_proxy();
  create_acl_provider();

  _handle.log(N_Event::INFO, "Connecting to filesystem...");
  _fs_proxy->connect();
  _handle.log(N_Event::INFO, "OK - Connected");
}
/*****************************************************************************/
string remove_trailing_slash(const string& path)
{
  return (*path.rbegin() == '/') ? path.substr(0, path.length()-1) : path;
}

/*****************************************************************************/
T_filesystem_config_ptr
T_filesystem_load::create_filesystem_config()
{
  LOG(INFO, 9) << "T_filesystem_load::create_filesystem_config()";

  T_filesystem_config_ptr conf;
  if (_fs_type == N_Uri::NFS)
    {
      T_mount_config_ptr mount_conf(new T_mount_config());

      mount_conf->remote_path = _configuration.get_string("root_directory");
      mount_conf->remote_path = remove_trailing_slash(mount_conf->remote_path);
      LOG(INFO, 4) << "Remote NFS path = " << mount_conf->remote_path;
      mount_conf->mount_point = _configuration.get_string("mount_point");
      mount_conf->mount_point = remove_trailing_slash(mount_conf->mount_point);
      LOG(INFO, 4) << "NFS Mount point = " << mount_conf->mount_point;
      if (_configuration.has_arg("mount_options"))
        {
          mount_conf->mount_options = _configuration.get_string("mount_options");
        }
      LOG(INFO, 4) << "NFS Mount options = " << mount_conf->mount_options;

      if (_configuration.has_arg("user_ids_to_names"))
        {
          map<string, string> uids = _configuration.get_string_map("user_ids_to_names");
          N_Security::fill_uid_gid_mapping(uids, mount_conf->users_mapping);
        }

      if (_configuration.has_arg("group_ids_to_names"))
        {
          map<string, string> gids = _configuration.get_string_map("group_ids_to_names");
          N_Security::fill_uid_gid_mapping(gids, mount_conf->groups_mapping);
        }

      LOG(INFO, 4) << "Users mapping : " << mount_conf->users_mapping.size()
                  << " mapping(s)";
      LOG(INFO, 4) << makeIteratorLogger(mount_conf->users_mapping.begin(),
                                        mount_conf->users_mapping.end());

      LOG(INFO, 4) << "Groups mapping : " << mount_conf->groups_mapping.size()
                  << " mapping(s)";
      LOG(INFO, 4) << makeIteratorLogger(mount_conf->groups_mapping.begin(),
                                        mount_conf->groups_mapping.end());
      conf = mount_conf;
    }
  else if (_fs_type == N_Uri::SMB)
    {
      T_samba_config_ptr smb_conf(new T_samba_config());

      if (not _configuration.has_arg("user"))
        {
          _handle.log(N_Event::FATAL, "Missing user argument in filter configuration");
        }
      smb_conf->user = _configuration.get_string("user");
      LOG(INFO, 4) << "Remote SMB user = " << smb_conf->user;

      if (not _configuration.has_arg("password"))
        {
          _handle.log(N_Event::FATAL, "Missing password argument in filter configuration");
        }
      smb_conf->password = _configuration.get_string("password");
      LOG(INFO, 4) << "Remote SMB password = "
                   << (smb_conf->password.empty() ? "" : "xxxxxx");

      if (_configuration.has_arg("workgroup"))
        {
          smb_conf->workgroup = _configuration.get_string("workgroup");
        }
      LOG(INFO, 4) << "Remote SMB workgroup = " << smb_conf->workgroup;

      smb_conf->share_name = _configuration.get_string("root_directory");
      LOG(INFO, 4) << "Remote SMB share name = " << smb_conf->share_name;

      if (_configuration.has_arg("user_ids_to_names"))
        {
          smb_conf->add_sid_mappings(_configuration.get_string_map("user_ids_to_names"),
                                    N_Security::USER);
        }
      else
        {
          _handle.log(N_Event::WARNING, "No SID to user names mapping defined");
        }

      if (_configuration.has_arg("group_ids_to_names"))
        {
          smb_conf->add_sid_mappings(_configuration.get_string_map("group_ids_to_names"),
                                    N_Security::GROUP);
        }
      else
        {
          _handle.log(N_Event::WARNING, "No SID to group names mapping defined");
        }

      LOG(INFO, 4) << "SID mappings: " << smb_conf->sid_mapping.size() << " mapping(s)"
                   << ": " << smb_conf->sid_mapping;

      conf = smb_conf;
    }

  // Common part

  conf->fs_type = _fs_type;
  conf->remote_host = _configuration.get_string("host");
  LOG(INFO, 4) << "Remote host = " << conf->remote_host;

  bool case_sensitive_filters = (_fs_type == N_Uri::SMB) ? false : true;
  _path_filter.reset(new T_path_filter(case_sensitive_filters));

  if (_configuration.has_arg("exclude"))
    {
      list<string> exclude_filter = _configuration.get_string_list("exclude");
      _path_filter->set_excluded_patterns(exclude_filter);
      LOG(INFO, 4) << "Exclude : " << exclude_filter.size() << " pattern(s)";
    }

  if (_configuration.has_arg("include"))
    {
      list<string> include_filter = _configuration.get_string_list("include");
      _path_filter->set_included_patterns(include_filter);
      LOG(INFO, 4) << "Include : " << include_filter.size() << " pattern(s)";
    }

  return conf;
}

/*****************************************************************************/
void T_filesystem_load::create_filesystem_proxy()
{
  T_filesystem_config_ptr fs_config = create_filesystem_config();
  switch(_fs_type)
    {
    case N_Uri::NFS:
      _fs_proxy.reset(new T_mounted_filesystem(fs_config));
      break;
    case N_Uri::SMB:
      _fs_proxy.reset(new T_samba_filesystem(fs_config));
      break;
    default:
      throw E_error("Invalid filesystem type");
    }
}

/*****************************************************************************/
void T_filesystem_load::create_acl_provider()
{
  switch(_fs_type)
  {
  case N_Uri::NFS:
    _acl_provider.reset(new T_mount_acl(
        dynamic_cast<T_mounted_filesystem&>(*_fs_proxy)));
    break;
  case N_Uri::SMB:
    _acl_provider.reset(new T_samba_acl(
        dynamic_cast<T_samba_filesystem&>(*_fs_proxy)));
    break;
  default:
    throw E_error("Invalid filesystem type");
  }
}

/*****************************************************************************/
void T_filesystem_load::process(AFS::PaF::Document& doc)
{
  LOG(INFO, 9) << "T_filesystem_load::process()";
  // By default, suppose KO
  doc.set_status(N_PaF::KO);

  N_Uri::T_uri  uri(doc.get_uri());

  switch (uri.protocol())
    {
    case N_Uri::NFS:
    case N_Uri::SMB:
      if (uri.protocol() == _fs_type)
        {
          process_uri(uri, doc);
        }
      else
        {
          LOG(INFO, 4) << "Uri protocol does not match filesystem type : "
                       << uri.get_raw_uri() << " vs "
                       << N_Uri::Protocol_Name(_fs_type)
                       << " - Skipping doc.";
        }
      break;
    default:
      LOG(INFO, 4) << "Uri protocol not managed - Skipping doc.";
      doc.set_status(N_PaF::OK);
    }

  process_deleted_files();

  // If I am here, all is OK
  LOG(INFO, 9) << "End of process !";
}

/*****************************************************************************/
void
T_filesystem_load::process_deleted_files()
{
  // Get candidates for deletion
  string paf_id_str = N_String::to_string(
      AFS::PaF::Pipe::pipe().get_current_PaF_id());
  auto_ptr<AFS::PaF::DocumentQueue> docs
    = _handle.get_where(
        "protocol = " + N_Uri::Protocol_Name(_fs_type)
        +" and status != DELETED and PaFId < "+paf_id_str);

  _handle.log(N_Event::INFO, "Will inspect " + N_String::to_string(docs->size())
              + " documents for suppression");

  // Check if candidates were deleted and add to a set if yes
  set<string> uris_to_delete;
  while (not docs->empty())
    {
      auto_ptr<AFS::PaF::Document> doc = docs->pop();
      string doc_uri = doc->get_uri();
      if (doc->get_status() == N_PaF::EOL)
        {
          if (to_be_deleted(*get_document_url(*doc)))
            {
              _handle.log(N_Event::INFO,
                          "Delete from PaF: " + doc_uri,
                          N_Event::VERBOSE);
              uris_to_delete.insert(doc_uri);
            }
        }
      else
        {
          LOG(INFO, 4) << "Delete skipped: " << doc_uri
                       << " (status = " << N_PaF::Status_Name(doc->get_status()) << ")";
        }
    }
  _handle.log(N_Event::INFO, "Documents suppression inspection finished, "
                + N_String::to_string(uris_to_delete.size())
                + " document(s) must be deleted.");

  if (uris_to_delete.size() != 0)
    {
      _handle.delete_documents(uris_to_delete);
      _stats._nb_deleted_files = uris_to_delete.size();
    }

  _handle.log(N_Event::INFO, "Documents suppression completed.");
}

/*****************************************************************************/
bool
T_filesystem_load::to_be_deleted(const T_url& url)
{
  bool filtered = not _path_filter->accept(url.get_local_path());
  bool existing = _fs_proxy->check_if_file_exists(url);
  LOG(INFO, 5) << "Check before delete: " << url.get_local_path()
               << " : filtered = " << filtered
               << " / existing = " << existing;
  return (filtered || not existing);
}

/*****************************************************************************/
void 
T_filesystem_load::process_uri(N_Uri::T_uri& uri,
                               AFS::PaF::Document& doc)
{
  _handle.log(N_Event::INFO,
              "RECEIVED URI to load: " + uri.get_raw_uri());
  T_url_ptr url = _fs_proxy->create_url(uri);

  // Check if URI is a directory or a file
  if (_fs_proxy->check_if_dir_exists(*url))
    {
      process_directory(*url, doc);
    }
  else
    {
      process_file(*url, doc);
    }
}

/*****************************************************************************/
void 
T_filesystem_load::process_file(const T_url& file_url,
                                AFS::PaF::Document& doc)
{
  string file_local_path = file_url.get_local_path();
  _handle.log(N_Event::INFO, "LOADING file: " + file_local_path);

  try
    {
      if (!doc.has_layer(N_PaF::N_Layer::CONTENTS))
        {
          ++_stats._nb_new_files;
        }
      else
        {
          ++_stats._nb_updated_files;
        }

      add_contents_layer(file_url, doc);
      if (AFS::PaF::Pipe::pipe().is_secured())
        {
          add_acl_layer(file_url, doc);
          add_sar_layer(file_url, doc);
        }
      doc.set_status(N_PaF::OK);
    }
  catch(E_error& e)
    {
      _handle.log(N_Event::ERROR, "Could not load file: "
                                  + file_url.get_local_path()
                                  + " [" + e.what() + "]");
    }
  catch(...)
    {
      _handle.log(N_Event::ERROR, "Could not load file: "
                                  + file_url.get_local_path());
    }
}

/*****************************************************************************/
void
T_filesystem_load::add_contents_layer(const T_url& url, 
                               AFS::PaF::Document& doc)
{
  if ((!doc.has_layer(N_PaF::N_Layer::CONTENTS)
      || is_layer_obsolete(N_PaF::N_Layer::CONTENTS,
                           doc,
                           _fs_proxy->read_file_mtime(url))))
  {
    T_binary_string data;
    _fs_proxy->read_file_content(url, data);
    doc.set_layer(data.get_data(), _output_type);
  }
}

/*****************************************************************************/
void 
T_filesystem_load::add_acl_layer(const T_url& url, 
                                 AFS::PaF::Document& doc)
{
  if ((!doc.has_layer(N_PaF::N_Layer::ACL)
      || is_layer_obsolete(N_PaF::N_Layer::ACL, 
                           doc, 
                           _fs_proxy->read_file_ctime(url))))
    {
      try
        {
          ACL file_acl = (*_acl_provider)(url.get_local_path());
          doc.set_protobuf_layer(file_acl, N_PaF::N_Layer::ACL);
        }
      catch (E_system& e)
        {
          doc.set_status(N_PaF::KO);
          LOG(ERROR, 1) << "Cannot read file/dir permissions: "
                        << url.get_local_path()
                        << " (" << e << ")";
        }
    }
  else
    {
      // add ACL to cache for SAR calculation
      auto_ptr<ACL> file_acl = doc.get_protobuf_layer<ACL>(N_PaF::N_Layer::ACL);
      _acl_provider->add(url.get_local_path(), *file_acl);
    }
}

/*****************************************************************************/
void
T_filesystem_load::add_sar_layer(const T_url& url, AFS::PaF::Document& doc)
{
  // SAR layer is always (re)computed as it depends on other documents ACLs
  LOG(INFO, 6) << "Compute SAR for " << doc.get_uri();
  try
    {
      SAR sar = _acl_provider->compute_sar_layer(url);
      doc.set_protobuf_layer(sar, N_PaF::N_Layer::SAR);
    }
  catch (E_system& e)
    {
      doc.set_status(N_PaF::KO);
      LOG(ERROR, 1) << "Cannot compute search access rights: "
                        << url.get_local_path()
                        << " (" << e << ")";
    }
}

/*****************************************************************************/
bool 
T_filesystem_load::is_layer_obsolete(N_PaF::N_Layer::Type type, 
                              AFS::PaF::Document& doc, 
                              time_t last_change)
{
  time_t last_revision = doc.get_layer(type)->get_last_modified().get_timestamp();
  return (last_change > last_revision);
}

/*****************************************************************************/
void T_filesystem_load::process_directory(const T_url& dir_url,
                                          AFS::PaF::Document& doc)
{
  set<string>   files;
  set<string>   subdirectories;

  _handle.log(N_Event::INFO,
              "Start processing directory: " + dir_url.get_local_path(),
              N_Event::VERBOSE);
  ++_stats._nb_directories;

  // Add trailing slash if missing
  string dir_path_s = dir_url.get_local_path();
  if (dir_path_s[dir_path_s.size() - 1] != '/')
    {
      dir_path_s += "/";
    }

  try
    {
      if (AFS::PaF::Pipe::pipe().is_secured())
        {
          add_acl_layer(dir_url, doc);
        }

      _fs_proxy->get_directory_files(dir_url, files);
      _fs_proxy->get_directory_subdirectories(dir_url, subdirectories);

      BOOST_FOREACH(string file_local_path, files)
        {
          if (_path_filter->accept(file_local_path))
            {
              T_url_ptr file_url = _fs_proxy->create_url(file_local_path);
              auto_ptr< AFS::PaF::Document> doc = get_or_create_document(*file_url);

              try
                {
                  process_file(*file_url, *doc);
                    // Send document to next filter
                  _handle.send(doc);
                }
              catch (E_system& e)
                {
                  if (_skip_non_readable_files)
                    {
                      LOG(WARNING, 2) << "Non readable file: " << file_local_path
                                      << "(" << e << ")"
                                      << " - skipped";
                    }
                  else
                    {
                      LOG(WARNING, 2) << "Non readable file: " << file_local_path
                                      << "(" << e << ")"
                                      << " - document KO";
                      _handle.send(doc);
                    }
                }
            }
          else
            {
              _handle.log(N_Event::INFO,
                          "Skipping ignored file: " + file_local_path,
                          N_Event::VERBOSE);
            }
        }
      BOOST_FOREACH(string subdir_local_path, subdirectories)
        {
          if (_path_filter->accept(subdir_local_path))
            {
              T_url_ptr subdir_url = _fs_proxy->create_url(subdir_local_path);
              auto_ptr< AFS::PaF::Document> doc = get_or_create_document(*subdir_url);
              process_directory(*subdir_url, *doc);
              // Send document to next filter
              _handle.send(doc);
            }
          else
            {
              _handle.log(N_Event::INFO,
                          "Skipping ignored directory: " + subdir_local_path,
                          N_Event::VERBOSE);
            }
        }

      doc.set_status(N_PaF::AUX);
    }
  catch(E_error& e)
      {
      _handle.log(N_Event::ERROR, "Could not load directory: "
                                  + dir_url.get_local_path()
                                  + " [" + e.what() + "]");
      doc.set_status(N_PaF::KO);
    }
  catch(...)
    {
      _handle.log(N_Event::ERROR, "Could not load directory: "
                                  + dir_url.get_local_path());
      doc.set_status(N_PaF::KO);
    }

  LOG(INFO, 6) << "End processing directory: " << dir_path_s;
}

/*****************************************************************************/
string 
T_filesystem_load::get_document_uri(const T_url& url)
{
  // PaF NFS document URI is the raw uri: "nfs://10.0.0.1/remote/full/path"
  return url.get_uri().get_raw_uri(true, false);
}

/*****************************************************************************/
T_url_ptr
T_filesystem_load::get_document_url(const AFS::PaF::Document& doc)
{
  return  _fs_proxy->create_url(N_Uri::T_uri(doc.get_uri()));
}

/*****************************************************************************/
auto_ptr< AFS::PaF::Document > 
T_filesystem_load::get_or_create_document(const T_url& url)
{
  string doc_uri = get_document_uri(url);
  
  auto_ptr<AFS::PaF::Document > doc = _handle.get_document(doc_uri);
  if (doc.get() == NULL)
    {
      LOG(INFO, 6) << "Creating new document: " << doc_uri;
      doc = _handle.new_document(doc_uri);
    }
  else
    {
      LOG(INFO, 6) << "Found existing document: " << doc_uri;
    }
  doc->set_status(N_PaF::KO);
  return doc;
}


/*****************************************************************************/
T_path_filter::T_path_filter(bool case_sensitive)
  : _is_case_sensitive(case_sensitive)
{
}

void
T_path_filter::set_excluded_patterns(const std::list< string >& patterns)
{
  _excludes = patterns;
  if (not _is_case_sensitive)
    {
      lowercase(_excludes);
    }
  LOG(INFO, 7) << "Excluded patterns: "
               << makeIteratorLogger(_excludes.begin(), _excludes.end());
}

void
T_path_filter::set_included_patterns(const std::list< string >& patterns)
{
  _includes = patterns;
  if (not _is_case_sensitive)
    {
      lowercase(_includes);
    }
  LOG(INFO, 7) << "Included patterns: "
               << makeIteratorLogger(_includes.begin(), _includes.end());
}

bool
T_path_filter::accept(const string& path)
{
  LOG(INFO, 7) << "Checking path filter for: " << path;
  return (not is_excluded(path)
          && ( (_includes.size() == 0) || is_included(path)));
}

void
T_path_filter::lowercase(list< string >& patterns)
{
  BOOST_FOREACH(string& pattern, patterns)
    {
      boost::to_lower(pattern);
    }
}

bool
T_path_filter::is_excluded(const string& path)
{
  return is_matching_one_pattern(path, _excludes);
}

bool
T_path_filter::is_included(const string& path)
{
  return is_matching_one_pattern(path, _includes);
}

bool
T_path_filter::is_matching_one_pattern(const string& path,
                                       const list< string >& patterns)
{
  if (patterns.empty()) return false;
  string path_to_compare(path);
  if (not _is_case_sensitive)
    {
      boost::to_lower(path_to_compare);
    }
  list<string>::const_iterator it = patterns.begin();
  while (it != patterns.end())
    {
      if (fnmatch(it->c_str(), path_to_compare.c_str(), 0) == 0)
        {
          LOG(INFO, 7) << "Path matched pattern: " << *it;
          return true;
        }
      else
        {
          LOG(INFO, 7) << "Path did not match pattern: " << *it;
        }
      ++it;
    }
  return false;
}


//
// End of file
//
