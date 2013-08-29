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
 * Description          : FILESYSTEM load filter
 *
 ***************************************************************************/
#ifndef _FILTER_FILESYSTEM_LOAD_H_
#define _FILTER_FILESYSTEM_LOAD_H_

#include "fs_url.h"
#include "fs_proxy.h"

#include <PaF/API/filter.h>
#include <COMMON/IO/io.h>
#include <AFS/SECURITY/tools.h>

#include <boost/ptr_container/ptr_container.hpp>
#include <boost/scoped_ptr.hpp>

class T_path_filter;

/*****************************************************************************/

struct T_filesystem_load_stats {
  uint32_t  _nb_directories;
  uint32_t  _nb_new_files;
  uint32_t  _nb_updated_files;
  uint32_t  _nb_deleted_files;
};

/*****************************************************************************/
class T_filesystem_load : public AFS::PaF::ProcessorFilter
{
public:
  T_filesystem_load(AFS::PaF::Configuration& configuration, 
             AFS::PaF::Handle& handle);
  
  virtual ~T_filesystem_load();
  virtual void init();
  virtual void process(AFS::PaF::Document& document);
  
protected:
  boost::scoped_ptr<T_filesystem_proxy> _fs_proxy;
  boost::scoped_ptr<T_path_filter>  _path_filter;
  N_Uri::Protocol                   _fs_type;
  N_PaF::N_Layer::Type              _output_type;
  boost::scoped_ptr<T_filesystem_acl> _acl_provider;
  bool _skip_non_readable_files;
  T_filesystem_load_stats  _stats;

  //! @brief Initializes the configuration of FILESYSTEM
  T_filesystem_config_ptr create_filesystem_config();

  //! @brief Initializes the FILESYSTEM proxy
  virtual void create_filesystem_proxy();

  //! @brief Creates the ACL provider
  virtual void create_acl_provider();

  //! @brief Process a FILESYSTEM uri (file or directory)
  void process_uri(N_Uri::T_uri& uri,
                   AFS::PaF::Document& doc);
  
  //! @brief Process a file
  void process_file(const T_url& url,
                    AFS::PaF::Document& doc);
  
  //! @brief Load file contents into the contents layer of the document
  void add_contents_layer(const T_url& url,
                          AFS::PaF::Document& doc);

  //! @brief Load file/dir permissions into the ACL layer of the document
  void add_acl_layer(const T_url& url,
                     AFS::PaF::Document& doc);

  //! @brief Compute and add SAR layer to document
  void add_sar_layer(const T_url& url,
                     AFS::PaF::Document& doc);

  //! @brief Checks if a document layer has a timestamp older than last_change
  //! @return true if document layer last modification date < last_change
  bool is_layer_obsolete( N_PaF::N_Layer::Type type,
                          AFS::PaF::Document& doc,
                          time_t last_change);

  //! @brief Process a directory
  void process_directory(const T_url& url,
                         AFS::PaF::Document& doc);

  //! @brief Processes deleted files
  void process_deleted_files();

  //! @brief Determine if a given file must be deleted or not
  bool to_be_deleted(const T_url& url);

  //! @brief Get the PaF document URI from the FILESYSTEM URL
  static string get_document_uri(const T_url& url);

  //! @brief Get the FILESYSTEM URL from the PaF document
  T_url_ptr get_document_url(const AFS::PaF::Document& doc);

  //! @brief Get or create a document for a file or directory
  auto_ptr< AFS::PaF::Document > 
  get_or_create_document(const T_url& url);

  //! @brief Log the filter statistics
  void log_stats();
};

/*****************************************************************************/
class T_path_filter
{
public:
  T_path_filter(bool case_sensitive = true);

  void set_excluded_patterns(const std::list<std::string>& patterns);
  void set_included_patterns(const std::list<std::string>& patterns);

  bool accept(const std::string& path);

private:
  bool _is_case_sensitive;
  std::list<std::string> _includes;
  std::list<std::string> _excludes;

private:
  void lowercase(std::list<std::string>& patterns);
  bool is_included(const std::string& path);
  bool is_excluded(const std::string& path);
  bool is_matching_one_pattern(const std::string& path,
                               const std::list<std::string>& patterns);
};

#endif // _FILTER_FILESYSTEM_LOAD_H_
