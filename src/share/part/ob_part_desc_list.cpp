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

#include "common/ob_obj_cast.h"
#include "share/part/ob_part_desc_list.h"

namespace oceanbase
{
namespace common
{

ObPartDescList::ObPartDescList() : part_array_ (NULL)
                                   , part_array_size_(0)
                                   , default_part_array_idx_(OB_INVALID_INDEX)
{
}

ObPartDescList::~ObPartDescList()
{
  // the memory is hold by allocator, will free if the allcator is reset
}

int64_t ObPartDescList::to_string(char *buf, const int64_t buf_len) const
{
  int64_t pos = 0;
  J_OBJ_START();

  J_KV("part_type", "list");
  J_COMMA();
  for (int64_t i = 0; i < part_array_size_; ++i) {
    J_KV("part_id", i, "part_array", part_array_[i]);
    J_COMMA();
  }

  J_OBJ_END();
  return pos;
}

ListPartition::ListPartition() : part_id_(0), rows_()
{
}

int ObPartDescList::get_part(ObNewRange &range,
                             ObIAllocator &allocator,
                             ObIArray<int64_t> &part_ids)
{
  int ret = OB_SUCCESS;
  part_ids.reset();

  if (OB_ISNULL(part_array_)
      || OB_UNLIKELY(part_array_size_ <= 0)) {
    ret = OB_INVALID_ARGUMENT;
    COMMON_LOG(WARN, "invalid argument, ", K_(part_array), K_(part_array_size), K(ret));
    // use the fisrt range as the type to cast
  } else if (1 != range.get_start_key().get_obj_cnt()) { // single value
    ret = OB_INVALID_ARGUMENT;
    COMMON_LOG(DEBUG, "list part should be single key",
                      "obj_cnt", range.get_start_key().get_obj_cnt(), K(ret));
  } else {
    ObObj &src_obj = const_cast<ObObj &>(range.get_start_key().get_obj_ptr()[0]);
    // use the first row cell as target obj
    ObObj &target_obj = const_cast<ObObj &>(part_array_[0].rows_[0].get_cell(0));
    if (OB_FAIL(cast_obj(src_obj, target_obj, allocator))) {
      COMMON_LOG(INFO, "fail to cast obj", K(src_obj), K(target_obj), K(ret));
    } else {
      bool found = false;
      for (int64_t i = 0; OB_SUCC(ret) && i < part_array_size_ && !found; ++i) {
        if (i == default_part_array_idx_) {
          continue;
        }
        for (int64_t j= 0; OB_SUCC(ret) && j < part_array_[i].rows_.count() && !found; ++j) {
          if (part_array_[i].rows_[j].get_count() == 0) {
            ret = OB_ERR_UNEXPECTED;
            COMMON_LOG(WARN, "no cells in the row", K(part_array_[i].rows_[j]), K(ret));
          } else if (src_obj == part_array_[i].rows_[j].get_cell(0)) {
            found = true;
            if (OB_FAIL(part_ids.push_back(part_array_[i].part_id_))) {
              COMMON_LOG(WARN, "fail to push part id", K(ret));
            }
          } // end found
        } // end for rows
      } // end for part_array
      if (!found && OB_INVALID_INDEX != default_part_array_idx_) {
        // if no row cell matches, use default partition
        COMMON_LOG(DEBUG, "will use default partition id", K(src_obj), K(ret));
        if (OB_FAIL(part_ids.push_back(part_array_[default_part_array_idx_].part_id_))) {
          COMMON_LOG(WARN, "fail to push part id", K(ret));
        }
      }
    }
  }
  return ret;
}

inline int ObPartDescList::cast_obj(ObObj &src_obj,
                                    const ObObj &target_obj,
                                    ObIAllocator &allocator)
{
  int ret = OB_SUCCESS;
  src_obj.set_collation_type(target_obj.get_collation_type());
  ObCastCtx cast_ctx(&allocator, NULL, CM_NULL_ON_WARN, target_obj.get_collation_type());
  // use src_obj as buf_obj
  if (OB_FAIL(ObObjCasterV2::to_type(target_obj.get_type(), cast_ctx, src_obj, src_obj))) {
    COMMON_LOG(INFO, "failed to cast obj", K(src_obj), K(target_obj), K(ret));
  }
  return ret;
}

} // end of common
} // end of oceanbase
