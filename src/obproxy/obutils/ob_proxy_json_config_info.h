/**
 * Copyright (c) 2021 OceanBase
 * OceanBase Database Proxy(ODP) is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#ifndef OBPROXY_JSON_CONFIG_INFO_H
#define OBPROXY_JSON_CONFIG_INFO_H

#include "lib/utility/ob_print_utils.h"
#include "lib/json/ob_json.h"
#include "lib/hash/ob_build_in_hashmap.h"
#include "utils/ob_proxy_lib.h"
#include "utils/ob_layout.h"
#include "proxy/route/ob_table_entry.h"

namespace oceanbase
{
namespace obproxy
{
namespace obutils
{

enum ObClusterRole
{
  INVALID_CLUSTER_ROLE = 0,
  PRIMARY = 1,
  STANDBY = 2,
};

const char *cluster_role_to_str(ObClusterRole role);

ObClusterRole str_to_cluster_role(const common::ObString &role_str);

class ObProxyBaseInfo
{
public:
  ObProxyBaseInfo() { }
  virtual ~ObProxyBaseInfo() { }

  virtual bool is_valid() const = 0;
  virtual int parse(const json::Value *value) = 0;

protected:
  static const int64_t OB_PROXY_MAX_CONFIG_STRING_LENGTH = 512;
  static const int64_t OB_PROXY_MAX_PASSWORD_LENGTH = 64;
  static const int64_t OB_PROXY_MAX_HTTP_MESSAGE_LENGTH = 16;
  static const int64_t OB_PROXY_MAX_VERSION_LENGTH = 32;
  static const int64_t OB_PROXY_MAX_NAME_STRING_LENGTH = 128;

private:
  DISALLOW_COPY_AND_ASSIGN(ObProxyBaseInfo);
};

class ObProxyConfigString : public ObProxyBaseInfo
{
public:
  ObProxyConfigString() : size_limit_(0) { reset(); }
  explicit ObProxyConfigString(const int64_t size_limit) : size_limit_(size_limit) { reset(); }
  virtual ~ObProxyConfigString() { }

  uint64_t hash(uint64_t seed = 0) const { return config_string_.hash(seed); }
  void reset() { config_string_.reset(); }
  bool empty() const { return config_string_.empty(); }
  bool is_valid() const { return !config_string_.empty(); }
  int32_t length() const { return config_string_.length(); }
  const char *ptr() const{ return config_string_str_; }
  char *ptr() { return config_string_str_; }
  int64_t buffer_size() const { return OB_PROXY_MAX_CONFIG_STRING_LENGTH - 1 ; }
  operator const common::ObString &() const { return config_string_; }
  const ObString& get_string() const { return config_string_; }

  int parse(const json::Value *value);

  void set_value(const common::ObString &value)
  {
    if (value.empty()) {
      MEMSET(config_string_str_, 0, OB_PROXY_MAX_CONFIG_STRING_LENGTH);
      config_string_.reset();
    } else {
      int32_t copy_len = std::min(value.length(), static_cast<int32_t>(OB_PROXY_MAX_CONFIG_STRING_LENGTH - 1));
      MEMCPY(config_string_str_, value.ptr(), copy_len);
      config_string_str_[value.length()] = '\0';
      config_string_.assign_ptr(config_string_str_, copy_len);
    }
  }

  void set_value(const int64_t len, const char *value)
  {
    int64_t copy_len = std::min(len, OB_PROXY_MAX_CONFIG_STRING_LENGTH - 1);
    if (OB_LIKELY(NULL != value) && OB_LIKELY(copy_len > 0)) {
      MEMCPY(config_string_str_, value, copy_len);
      config_string_str_[copy_len] = '\0';
      config_string_.assign_ptr(config_string_str_, static_cast<int32_t>(copy_len));
    }
  }

  common::ObString after(const char p) const { return config_string_.after(p); }

  int assign(const ObProxyConfigString &other)
  {
    int ret = common::OB_SUCCESS;
    if (OB_LIKELY(this != &other)) {
      set_value(other.config_string_);
    }
    return ret;
  }

  bool operator ==(const ObProxyConfigString &other) const { return config_string_ == other.config_string_; }
  bool operator !=(const ObProxyConfigString &other) const { return !(*this == other); }
  DECLARE_TO_STRING;

public:
  common::ObString config_string_;

private:
  int64_t size_limit_;
  char config_string_str_[OB_PROXY_MAX_CONFIG_STRING_LENGTH];
  DISALLOW_COPY_AND_ASSIGN(ObProxyConfigString);
};

class ObProxyConfigUrl : public ObProxyBaseInfo
{
public:
  ObProxyConfigUrl() : url_str_(NULL) { reset(); }
  virtual ~ObProxyConfigUrl() { }

  void reset()
  {
    url_.reset();
    url_str_ = NULL;
  }
  int parse(const json::Value *value)
  {
    UNUSED(value);
    return common::OB_SUCCESS;
  }
  int parse(const json::Value *value, common::ObIAllocator &allocator);
  int set_url(char *buffer, const int64_t len);

  int32_t length() const { return url_.length(); }
  const char *ptr() const{ return url_.ptr(); }
  const char *buf_ptr() const{ return url_str_; }
  operator const common::ObString &() const { return url_; }

  bool is_valid() const { return !url_.empty(); }
  bool operator ==(const ObProxyConfigUrl &other) const { return url_ == other.url_; }
  bool operator !=(const ObProxyConfigUrl &other) const { return !(*this == other); }
  DECLARE_TO_STRING;

public:
  common::ObString url_;

private:
  char *url_str_;
  DISALLOW_COPY_AND_ASSIGN(ObProxyConfigUrl);
};

class ObProxyNameString : public ObProxyBaseInfo
{
public:
  ObProxyNameString() : size_limit_(0) { reset(); }
  explicit ObProxyNameString(const int64_t size_limit) : size_limit_(size_limit) { reset(); }
  virtual ~ObProxyNameString() { }

  void reset() { name_string_.reset(); }
  bool empty() const { return name_string_.empty(); }
  bool is_valid() const { return !name_string_.empty(); }
  int32_t length() const { return name_string_.length(); }
  const char *ptr() const{ return name_string_str_; }
  operator const common::ObString &() const { return name_string_; }
  uint64_t hash() const { return name_string_.hash(); }
  void to_lower_case()
  {
    for (int32_t i = 0; i < length(); i++) {
      name_string_str_[i] = static_cast<char>(tolower(name_string_str_[i]));
    }
  }


  int parse(const json::Value *value);

  void set_value(const common::ObString &value)
  {
    if (value.empty()) {
      name_string_.reset();
    } else {
      MEMCPY(name_string_str_, value.ptr(), value.length());
      name_string_str_[value.length()] = '\0';
      name_string_.assign_ptr(name_string_str_, value.length());
    }
  }

  int assign(const ObProxyNameString &other)
  {
    int ret = common::OB_SUCCESS;
    if (OB_LIKELY(this != &other)) {
      set_value(other.name_string_);
    }
    return ret;
  }

  bool operator ==(const ObProxyNameString &other) const { return name_string_ == other.name_string_; }
  bool operator !=(const ObProxyNameString &other) const { return !(*this == other); }
  TO_STRING_KV("string", name_string_);

public:
  common::ObString name_string_;

private:
  int64_t size_limit_;
  char name_string_str_[OB_PROXY_MAX_NAME_STRING_LENGTH];
  DISALLOW_COPY_AND_ASSIGN(ObProxyNameString);
};

template<int64_t size = 4096>
class ObProxySizeConfigString
{
public:
  ObProxySizeConfigString() { reset(); }
  virtual ~ObProxySizeConfigString() { }

  uint64_t hash(uint64_t seed = 0) const { return config_string_.hash(seed); }
  void reset() { config_string_.reset(); }
  bool empty() const { return config_string_.empty(); }
  bool is_valid() const { return !config_string_.empty(); }
  int32_t length() const { return config_string_.length(); }
  const char *ptr() const{ return config_string_str_; }
  operator const common::ObString &() const { return config_string_; }
  const ObString& get_string() const { return config_string_; }

  int parse(const json::Value *value);

  void set_value(const common::ObString &value)
  {
    if (value.empty()) {
      MEMSET(config_string_str_, 0, size);
      config_string_.reset();
    } else {
      int32_t copy_len = std::min(value.length(), static_cast<int32_t>(size - 1));
      MEMCPY(config_string_str_, value.ptr(), copy_len);
      config_string_str_[value.length()] = '\0';
      config_string_.assign_ptr(config_string_str_, copy_len);
    }
  }

  common::ObString after(const char p) const { return config_string_.after(p); }

  TO_STRING_KV("string", config_string_);

public:
  common::ObString config_string_;

private:
  char config_string_str_[size];
  DISALLOW_COPY_AND_ASSIGN(ObProxySizeConfigString);
};

class ObProxyIDCInfo
{
public:
  ObProxyIDCInfo() : idc_hash_(0), idc_name_(common::MAX_PROXY_IDC_LENGTH),
                     region_name_(common::MAX_REGION_LENGTH) { }
  ~ObProxyIDCInfo() { }

  void reset()
  {
    idc_hash_ = 0;
    idc_name_.reset();
    region_name_.reset();
  }
  bool is_valid() const { return idc_name_.is_valid() && region_name_.is_valid(); }
  int assign(const ObProxyIDCInfo &other)
  {
    reset();
    idc_name_.set_value(other.idc_name_);
    region_name_.set_value(other.region_name_);
    idc_hash_ = other.idc_hash_;
    return common::OB_SUCCESS;
  }

  bool operator ==(const ObProxyIDCInfo &other) const
  {
    return (idc_name_ == other.idc_name_ && region_name_ == other.region_name_);
  }
  bool operator !=(const ObProxyIDCInfo &other) const { return !(*this == other); }

  TO_STRING_KV(K_(idc_hash), K_(idc_name), K_(region_name));

public:
  static const int64_t MAX_CLUSTER_IDC_COUNT = 16;
  uint64_t idc_hash_;
  ObProxyNameString idc_name_;
  ObProxyNameString region_name_;

  LINK(ObProxyIDCInfo, idc_link_);

private:
  DISALLOW_COPY_AND_ASSIGN(ObProxyIDCInfo);
};

typedef common::ObSEArray<proxy::ObProxyReplicaLocation, proxy::ObProxyPartitionLocation::OB_PROXY_REPLICA_COUNT> LocationList;
typedef common::ObSEArray<ObProxyIDCInfo, ObProxyIDCInfo::MAX_CLUSTER_IDC_COUNT> ObProxyIDCList;
class ObProxySubClusterInfo
{
public:
  ObProxySubClusterInfo() : is_used_(false), role_(INVALID_CLUSTER_ROLE), cluster_id_(common::OB_INVALID_CLUSTER_ID),
                            rs_list_hash_(0), create_failure_count_(0) {}
  ~ObProxySubClusterInfo() {}

  void destroy() { op_free(this); }

  bool is_web_rs_list_changed(const LocationList &other) const
  {
    bool bret = false;
    if (web_rs_list_.count() != other.count()) {
      bret = true;
    } else {
      for (int64_t i = 0; !bret && i < web_rs_list_.count(); ++i) {
        if (web_rs_list_[i] != other[i]) {
          bret = true;
        }
      }
    }
    return bret;
  }

  bool is_cluster_role_changed(const common::ObString &role_str) const
  {
    bool bret = false;
    if (str_to_cluster_role(role_str) != role_) {
      bret = true;
    }
    return bret;
  }

  bool is_idc_list_changed(const ObProxyIDCList &other) const
  {
    bool bret = false;
    if (idc_list_.count() != other.count()) {
      bret = true;
    } else {
      for (int64_t i = 0; !bret && i < idc_list_.count(); ++i) {
        if (idc_list_[i].idc_hash_ != other[i].idc_hash_) {
          bret = true;
        }
      }
    }
    return bret;
  }
  int update_rslist(const LocationList &rs_list, const uint64_t hash = 0);
  int get_idc_region(const common::ObString &idc_name,
                     ObProxyNameString &region_name) const;
  void reuse_rslist() { web_rs_list_.reuse(); }
  void reuse_idc_list() { idc_list_.reuse(); }
  
  DECLARE_TO_STRING;

public:
  bool is_used_;
  ObClusterRole role_;
  int64_t cluster_id_;
  uint64_t rs_list_hash_;
  int64_t create_failure_count_;//if cnt > 3 && rslist exist , reset rslist;
  LocationList web_rs_list_;
  ObProxyIDCList idc_list_;
  LINK(ObProxySubClusterInfo, sub_cluster_link_);

private:
  DISALLOW_COPY_AND_ASSIGN(ObProxySubClusterInfo);
};

class ObProxyClusterInfo : public ObProxyBaseInfo
{
public:
  ObProxyClusterInfo();
  virtual ~ObProxyClusterInfo() { }

  void destroy();
  void reset();

  int parse(const json::Value *value)
  {
    UNUSED(value);
    return common::OB_SUCCESS;
  }
  int parse(const json::Value *value, common::ObIAllocator &allocator);

  bool is_valid() const { return cluster_name_.is_valid(); }

  // Attention!!! assign function only copy cluster name and web_rs_list, if u want to get
  // cluster url, u should explicitly use a buffer to copy it
  int assign(const ObProxyClusterInfo &other)
  {
    int ret = common::OB_SUCCESS;
    if (OB_LIKELY(this != &other)) {
      if (OB_FAIL(cluster_name_.assign(other.cluster_name_))) {
        _PROXY_LOG(WARN, "cluster_info_key assign error, ret = [%d]", ret);
      } else {
        master_cluster_id_ = other.master_cluster_id_;
      }
    }
    return ret;
  }

  int get_idc_region(const common::ObString &idc_name, ObProxyNameString &region_name, const int64_t cluster_id) const;

  bool operator ==(const ObProxyClusterInfo &other) const
  {
    //NOTE::As we can only get cluster_name and rs_url from config server json web,
    //      here we only compare them instead of all the member
    return (cluster_name_ == other.cluster_name_
            && rs_url_ == other.rs_url_);
  }
  bool operator !=(const ObProxyClusterInfo &other) const { return !(*this == other); }

  static uint64_t get_server_list_hash(const LocationList &rs_list);
  int get_rs_list_hash(const int64_t cluster_id, uint64_t &hash);
  int64_t get_rs_list_count() const;
  int64_t get_idc_list_count() const;
  int64_t get_sub_cluster_count() const { return sub_ci_map_.count(); }

  int get_sub_cluster_info(const int64_t cluster_id, ObProxySubClusterInfo *&sub_cluster_info) const;

  DECLARE_TO_STRING;

public:
  static const int64_t RESET_RS_LIST_FAILURE_COUNT = 3;
  static const int64_t OB_REGION_HASH_BUCKET_SIZE = 16;

  // cluster_id ---> rs list
  struct ObSubClusterInfoHashing
  {
    typedef const int64_t &Key;
    typedef ObProxySubClusterInfo Value;
    typedef ObDLList(ObProxySubClusterInfo, sub_cluster_link_) ListHead;

    static uint64_t hash(Key key) { return common::murmurhash(&key, sizeof(key), 0); }
    static Key key(Value *value) { return value->cluster_id_; }
    static bool equal(Key lhs, Key rhs) { return lhs == rhs; }
  };
  typedef common::hash::ObBuildInHashMap<ObSubClusterInfoHashing, OB_REGION_HASH_BUCKET_SIZE> SubCIHashMap;

  ObProxyConfigUrl rs_url_;
  ObProxyConfigString cluster_name_;
  int64_t master_cluster_id_;
  SubCIHashMap sub_ci_map_;
  LINK(ObProxyClusterInfo, cluster_link_);

private:
  DISALLOW_COPY_AND_ASSIGN(ObProxyClusterInfo);
};

class ObProxyMetaTableInfo : public ObProxyBaseInfo
{
public:
  ObProxyMetaTableInfo();
  virtual ~ObProxyMetaTableInfo() { }

  void reset();
  int parse(const json::Value *value)
  {
    UNUSED(value);
    return common::OB_SUCCESS;
  }
  int parse(const json::Value *value, common::ObIAllocator &allocator);

  bool is_valid() const
  {
    return (db_.is_valid() && username_.is_valid() && password_.is_valid() && cluster_info_.is_valid());
  }

  int assign(const ObProxyMetaTableInfo &other)
  {
    int ret = common::OB_SUCCESS;
    if (OB_LIKELY(this != &other)) {
      if (OB_FAIL(db_.assign(other.db_))) {
        _PROXY_LOG(WARN, "db assign error, ret = [%d]", ret);
      } else if (OB_FAIL(username_.assign(other.username_))) {
        _PROXY_LOG(WARN, "username assign error, ret = [%d]", ret);
      } else if (OB_FAIL(password_.assign(other.password_))) {
        _PROXY_LOG(WARN, "password assign error, ret = [%d]", ret);
      } else if (OB_FAIL(real_cluster_name_.assign(other.real_cluster_name_))) {
        _PROXY_LOG(WARN, "real_cluster_name assign error, ret = [%d]", ret);
      } else if (OB_FAIL(cluster_info_.assign(other.cluster_info_))) {
        _PROXY_LOG(WARN, "meta_cluster_info assign error, ret = [%d]", ret);
      }
    }
    return ret;
  }

  bool operator ==(const ObProxyMetaTableInfo &other) const
  {
    //NOTE::As we can not get real_cluster_name_ from config server json web,
    //      here we only compare the whole member when real_cluster_names are available
    return db_ == other.db_
           && username_ == other.username_
           && password_ == other.password_
           && ((real_cluster_name_.empty() || other.real_cluster_name_.empty())
                || (real_cluster_name_ == other.real_cluster_name_))
           && cluster_info_ == other.cluster_info_;
  }

  bool operator !=(const ObProxyMetaTableInfo &other) const { return !(*this == other); }
  DECLARE_TO_STRING;

private:
  // if username contains cluster name :
  // 1. cluster name == MetaDataBase, we will trim cluster name
  // 2. cluster name != MetaDataBase, we will print warn log and not use this json config info
  int check_and_trim_username();

public:
  ObProxyConfigString db_;
  ObProxyConfigString username_;
  ObProxyConfigString password_;
  ObProxyConfigString real_cluster_name_;
  ObProxyClusterInfo cluster_info_;

private:
  DISALLOW_COPY_AND_ASSIGN(ObProxyMetaTableInfo);
};

class ObProxyClusterArrayInfo : public ObProxyBaseInfo
{
public:
  ObProxyClusterArrayInfo();
  virtual ~ObProxyClusterArrayInfo();

  void destroy();
  int parse(const json::Value *value)
  {
    UNUSED(value);
    return common::OB_SUCCESS;
  }
  int parse(const json::Value *value, common::ObIAllocator &allocator);

  bool is_valid() const;

  bool is_cluster_exists(const common::ObString &name) const;
  bool is_idc_list_exists(const common::ObString &name) const;

  int64_t count() const { return ci_map_.count(); }

  int get(const common::ObString &name, ObProxyClusterInfo *&cluster_info) const;

  int parse_ob_region(const json::Value *json_value,
                      const ObProxyConfigString &root_service_url_template,
                      common::ObIAllocator &allocator);

  DECLARE_TO_STRING;

private:
  int generate_cluster_url(const common::ObString &cluster_name,
                           const common::ObString &root_service_url_template,
                           ObProxyConfigUrl &url,
                           common::ObIAllocator &allocator);

public:
  static const int64_t HASH_BUCKET_SIZE = 512;

  struct ObClusterInfoHashing
  {
    typedef const common::ObString &Key;
    typedef ObProxyClusterInfo Value;
    typedef ObDLList(ObProxyClusterInfo, cluster_link_) ListHead;

    static uint64_t hash(Key key) { return key.hash(); }
    static Key key(Value *value) { return value->cluster_name_.config_string_; }
    static bool equal(Key lhs, Key rhs) { return lhs == rhs; }
  };
  typedef common::hash::ObBuildInHashMap<ObClusterInfoHashing, HASH_BUCKET_SIZE> CIHashMap;

public:
  common::ObString default_cluster_name_;
  CIHashMap ci_map_;

private:
  DISALLOW_COPY_AND_ASSIGN(ObProxyClusterArrayInfo);
};

class ObProxyDataInfo : ObProxyBaseInfo
{
public:
  ObProxyDataInfo();
  virtual ~ObProxyDataInfo() { }

  void reset();
  int parse(const json::Value *value)
  {
    UNUSED(value);
    return common::OB_SUCCESS;
  }
  int parse(const json::Value *value, common::ObIAllocator &allocator);

  //parse version to check if the version is changed, if not, no need to parse anymore
  static int parse_version(const json::Value *root, const common::ObString &version);

  bool is_valid() const { return version_.is_valid() && bin_url_.is_valid() && meta_table_info_.is_valid() && cluster_array_.is_valid(); }

  DECLARE_TO_STRING;

public:
  ObProxyConfigString version_;
  ObProxyConfigUrl bin_url_;
  ObProxyMetaTableInfo meta_table_info_;
  ObProxyClusterArrayInfo cluster_array_;
  // for new ocp protocol v2
  ObProxyConfigString root_service_url_template_;
  ObProxyConfigString root_service_url_template_v2_;

private:
  DISALLOW_COPY_AND_ASSIGN(ObProxyDataInfo);
};

struct ObProxyLoginInfo
{
  ObProxyLoginInfo() {}
  ~ObProxyLoginInfo() {}
  bool is_valid() const{ return db_.is_valid() && username_.is_valid() && password_.is_valid(); }
  TO_STRING_KV(K_(username), K_(db));// can not print passwd into log

  ObProxyConfigString db_;
  ObProxyConfigString username_;
  ObProxyConfigString password_;
};

/* ObProxyJsonConfigInfo contains all the infos parsed from proxy config file,
 * which is fetched by config server processor. It will provide the config server
 * with ob cluster array info, meta db table info and proxy bin url. Meanwhile,
 * Config Server will update ObProxyJsonConfigInfo periodically if the version of
 * json config file changed.
*/

class ObProxyJsonConfigInfo : public ObProxyBaseInfo, public common::ObRefCountObj
{
public:
  ObProxyJsonConfigInfo();
  virtual ~ObProxyJsonConfigInfo() { }

  void reset();
  int parse(const json::Value *value);
  int rslist_to_json(char *buf, const int64_t buf_len, int64_t &data_len);
  int parse_local_rslist(const json::Value *root);
  int parse_remote_rslist(const json::Value *root,
                          const common::ObString &appname,
                          const int64_t cluster_id,
                          LocationList &web_rslist,
                          const bool need_update_dummy_entry = true);
  int parse_rslist_array_data(const json::Value *root, const common::ObString &appname,
                              const bool is_from_local = false);
  int parse_rslist_data(const json::Value *root,
                        const common::ObString &appname,
                        int64_t &cluster_id,
                        LocationList &web_rslist,
                        bool &is_primary,
                        common::ObString &cluster_name,
                        bool is_from_local = false,
                        const bool need_update_dummy_entry =true);
  int parse_rslist_item(const json::Value *root, const common::ObString &appname,
                        LocationList &web_rslist, const bool is_readonly_zone);
  int get_rslist_file_max_size(int64_t &max_size);

  int idc_list_to_json(char *buf, const int64_t buf_len, int64_t &data_len);
  int parse_local_idc_list(const json::Value *root);
  static int parse_remote_idc_list(const json::Value *root, common::ObString &cluster_name,
                                   int64_t &cluster_id, ObProxyIDCList &idc_list);
  static int parse_idc_list_data(const json::Value *root, common::ObString &cluster_name,
                                 int64_t &cluster_id, ObProxyIDCList &idc_list);
  static int parse_idc_list_item(const json::Value *root, ObProxyIDCList &idc_list);
  int get_idc_list_file_max_size(int64_t &max_size);

  bool is_valid() const { return data_info_.is_valid(); }

  bool is_meta_db_changed(const ObProxyJsonConfigInfo &other) const
  {
    return data_info_.meta_table_info_.is_valid()
           && data_info_.meta_table_info_ != other.data_info_.meta_table_info_;
  }

  int copy_bin_url(char *bin_url, const int64_t len) const;
  int get_meta_table_info(ObProxyMetaTableInfo &table_info) const
  {
    int ret = common::OB_SUCCESS;
    if (OB_FAIL(table_info.assign(data_info_.meta_table_info_))) {
      _PROXY_LOG(WARN, "fail to assign meta table info, ret = [%d]", ret);
    }
    return ret;
  }

  int get_meta_table_username(ObProxyConfigString &username) const
  {
    int ret = common::OB_SUCCESS;
    if (OB_FAIL(username.assign(data_info_.meta_table_info_.username_))) {
      _PROXY_LOG(WARN, "fail to assign meta table username, ret = [%d]", ret);
    }
    return ret;
  }

  const common::ObString &get_real_meta_cluster_name() const { return data_info_.meta_table_info_.real_cluster_name_; }
  int64_t get_meta_cluster_id() const { return data_info_.meta_table_info_.cluster_info_.master_cluster_id_; }

  int get_meta_table_login_info(ObProxyLoginInfo &info) const
  {
    int ret = common::OB_SUCCESS;
    if (OB_FAIL(info.username_.assign(data_info_.meta_table_info_.username_))) {
      PROXY_LOG(WARN, "fail to assign meta table username", K(ret));
    } else if (OB_FAIL(info.password_.assign(data_info_.meta_table_info_.password_))) {
      PROXY_LOG(WARN, "fail to assign meta table password", K(ret));
    } else if (OB_FAIL(info.db_.assign(data_info_.meta_table_info_.db_))) {
      PROXY_LOG(WARN, "fail to assign meta table db", K(ret));
    } else {/*do nothing*/}
    return ret;
  }

  const ObProxyConfigUrl &get_bin_url() const { return data_info_.bin_url_; }
  const ObProxyMetaTableInfo &get_meta_table_info() const { return data_info_.meta_table_info_; }
  const ObProxyClusterArrayInfo &get_cluster_array() const { return data_info_.cluster_array_; }
  const ObProxyDataInfo &get_data_info() const { return data_info_; }
  const common::ObString &get_data_version() const { return data_info_.version_; }

  void destroy_cluster_info() { data_info_.cluster_array_.destroy(); }

  bool is_real_meta_cluster_exist() const { return !data_info_.meta_table_info_.real_cluster_name_.empty(); }
  bool is_cluster_exists(const common::ObString &cluster_name) const { return data_info_.cluster_array_.is_cluster_exists(cluster_name); }
  int64_t get_cluster_count() const { return data_info_.cluster_array_.count(); }
  bool is_cluster_idc_list_exists(const common::ObString &cluster_name, const int64_t cluster_id) const;

  int get_cluster_info(const common::ObString &cluster_name, ObProxyClusterInfo *&cluster_info) const;
  int get_sub_cluster_info(const common::ObString &cluster_name, const int64_t cluster_id,
                           ObProxySubClusterInfo *&sub_cluster_info) const;
  int get_next_master_cluster_info(const common::ObString &cluster_name,
                                   ObProxySubClusterInfo *&sub_cluster_info,
                                   bool &is_master_changed);
  int reset_is_used_flag(const common::ObString &cluster_name);
  int get_master_cluster_id(const common::ObString &cluster_name, int64_t &cluster_id) const;
  int get_rs_list_hash(const common::ObString &cluster_name, const int64_t cluster_id, uint64_t &rs_list_hash) const;
  int64_t inc_create_failure_count(const common::ObString &cluster_name, const int64_t cluster_id);

  //we use the first cluster info in cluster array as default
  int get_default_cluster_name(char *buf, const int64_t len) const;

  int add_default_cluster_info(ObProxyClusterInfo *cluster_info, const LocationList &web_rs_list);
  int add_to_list(const common::ObString &ip, const common::ObString &role,
                  const common::ObString &cluster_name, int64_t sql_port,
                  const common::ObReplicaType replica_type, LocationList &web_rs_list);

  int set_cluster_web_rs_list(const common::ObString &cluster_name, const int64_t cluster_id,
                              const LocationList &web_rs_list,
                              const common::ObString &role,
                              const uint64_t cur_rs_list_hash = 0);
  int set_master_cluster_id(const common::ObString &cluster_name, const int64_t cluster_id);
  int set_real_meta_cluster_name(const common::ObString &real_meta_cluster_name);
  int set_idc_list(const common::ObString &cluster_name, const int64_t cluster_id, const ObProxyIDCList &idc_list);
  int delete_cluster_rslist(const common::ObString &cluster_name, const int64_t cluster_id);
  int reset_create_failure_count(const common::ObString &cluster_name, const int64_t cluster_id);

  bool cluster_info_empty() const { return 0 == data_info_.cluster_array_.count(); }
  DECLARE_TO_STRING;

public:
  static const int64_t OP_LOCAL_NUM = 2;
  int64_t gmt_modified_;

private:
  event::ObFixedArenaAllocator<ObLayout::MAX_PATH_LENGTH> allocator_;
  ObProxyDataInfo data_info_;
  DISALLOW_COPY_AND_ASSIGN(ObProxyJsonConfigInfo);
};

class ObProxyObInstance : public ObProxyBaseInfo, public ObSharedRefCount
{
public:
  ObProxyObInstance() : ob_cluster_(OB_PROXY_MAX_NAME_STRING_LENGTH), ob_cluster_id_(0),
                        ob_tenant_(OB_PROXY_MAX_NAME_STRING_LENGTH), ob_tenant_id_(0),
                        role_(OB_PROXY_MAX_NAME_STRING_LENGTH) {}
  ~ObProxyObInstance() {}
  void free() { op_free(this); }
  int parse(const json::Value *value);
  void reset()
  {
    ob_cluster_.reset();
    ob_tenant_.reset();
    role_.reset();
    ob_cluster_id_ = 0;
    ob_tenant_id_ = 0;
  }
  bool is_valid() const
  {
    bool bret = true;
    if (ob_cluster_.empty() || ob_tenant_.empty() || role_.empty()) {
      bret = false;
    }
    return bret;
  }
  DECLARE_TO_STRING;
public:
  ObProxyConfigString ob_cluster_;
  int64_t ob_cluster_id_;
  ObProxyConfigString ob_tenant_;
  int64_t ob_tenant_id_;
  ObProxyConfigString role_;
private:
  DISALLOW_COPY_AND_ASSIGN(ObProxyObInstance);
};

class ObProxyLdgObInstacne : public ObProxyBaseInfo
{
public:
  ObProxyLdgObInstacne() : ldg_cluster_(OB_PROXY_MAX_NAME_STRING_LENGTH), cluster_id_(0),
                           ldg_tenant_(OB_PROXY_MAX_NAME_STRING_LENGTH), tenant_id_(0),
                           instance_array_(), hash_key_() {}
  ~ObProxyLdgObInstacne() { destroy(); }
  int parse(const json::Value *value);
  bool is_valid() const
  {
    bool bret = true;
    if (ldg_cluster_.empty() || ldg_tenant_.empty()) {
      bret = false;
    }
    return bret;
  }
  common::ObString& get_hash_key();
  void destroy()
  {
    for (int64_t i = 0; i < instance_array_.count(); i++) {
      ObProxyObInstance* &ob_instance = instance_array_.at(i);
      if (NULL != ob_instance) {
        ob_instance->dec_ref();
        ob_instance = NULL;
      }
    }
  }
public:
  ObProxyConfigString ldg_cluster_;
  int64_t cluster_id_;
  ObProxyConfigString ldg_tenant_;
  int64_t tenant_id_;
  ObSEArray<ObProxyObInstance*, 4> instance_array_;
  ObString hash_key_;
  LINK(ObProxyLdgObInstacne, ldg_ob_instance_link_);
  DECLARE_TO_STRING;
private:
  char hash_key_buf_[128];
private:
  DISALLOW_COPY_AND_ASSIGN(ObProxyLdgObInstacne);
};

class ObProxyLdgInfo : public ObProxyBaseInfo, public common::ObRefCountObj
{
public:
  ObProxyLdgInfo() : version_(OB_PROXY_MAX_CONFIG_STRING_LENGTH), ldg_instance_map_() {}
  ~ObProxyLdgInfo() { destroy(); }
  int parse(const json::Value *value);
  bool is_valid() const
  {
    bool bret = true;
    if (0 == ldg_instance_map_.count()) {
      bret = false;
    }
    return bret;
  }
  struct ObProxyLdgObInstacneHashing
  {
    typedef const common::ObString &Key;
    typedef ObProxyLdgObInstacne Value;
    typedef ObDLList(ObProxyLdgObInstacne, ldg_ob_instance_link_) ListHead;
    static uint64_t hash(Key key) { return key.hash(); }
    static Key key(Value *value) { return value->get_hash_key(); }
    static bool equal(Key lhs, Key rhs) { return lhs == rhs; }
  };
  int update_ldg_instance(ObProxyLdgObInstacne *ldg_instance);
  int get_ldg_instance(const common::ObString &key, ObProxyLdgObInstacne *&ldg_instance);
  
  int get_primary_role_instance(const ObString &tenant_name, const ObString &cluster_name, ObProxyObInstance* &instance);
  void destroy();
  typedef common::hash::ObBuildInHashMap<ObProxyLdgObInstacneHashing> LdgInstanceMap;
  DECLARE_TO_STRING;
private:
  ObProxyConfigString version_;
  LdgInstanceMap ldg_instance_map_;
  DISALLOW_COPY_AND_ASSIGN(ObProxyLdgInfo);
};

class ObProxyJsonUtils
{
public:
  static int parse_header(const json::Value *json_value, json::Value *&value);
  static int cluster_rslist_to_json(const ObProxyClusterInfo &cluster_info, char *buf, const int64_t buf_len, int64_t &pos,
                                    const char *real_meta_cluster = NULL);
  static int rslist_to_json(const LocationList &addr_list, const char *appname,
                            const int64_t cluster_id, const ObClusterRole role, char *buf, const int64_t buf_len, int64_t &pos,
                            const char *real_meta_cluster = NULL);
  static int cluster_idc_list_to_json(const ObProxyClusterInfo &cluster_info, char *buf, const int64_t buf_len, int64_t &pos,
                                      const char *real_meta_cluster = NULL);
  static int idc_list_to_json(const ObProxyIDCList &idc_list, const char *appname,
                              const int64_t cluster_id, char *buf, const int64_t buf_len, int64_t &pos,
                              const char *real_meta_cluster = NULL);

  static int check_config_info_type(const json::Value *json_value, const json::Type type);
  static int check_config_string(const common::ObString &value, int64_t size_limit);
};

}//end of namespace obutils
}//end of namespace obproxy
}//end of namespace oceanbase

#endif /* OBPROXY_JSON_CONFIG_INFO_H */
