/**
 * @file   writer.h
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017-2021 TileDB, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * This file defines class Writer.
 */

#ifndef TILEDB_WRITER_H
#define TILEDB_WRITER_H

#include <memory>
#include <set>
#include <unordered_map>

#include "tiledb/common/status.h"
#include "tiledb/sm/fragment/written_fragment_info.h"
#include "tiledb/sm/misc/types.h"
#include "tiledb/sm/query/dense_tiler.h"
#include "tiledb/sm/query/query_buffer.h"
#include "tiledb/sm/query/validity_vector.h"
#include "tiledb/sm/stats/stats.h"
#include "tiledb/sm/subarray/subarray.h"
#include "tiledb/sm/tile/tile.h"

using namespace tiledb::common;

namespace tiledb {
namespace sm {

class Array;
class ArraySchema;
class FragmentMetadata;
class StorageManager;

/** Processes write queries. */
class Writer {
 public:
  /* ********************************* */
  /*          TYPE DEFINITIONS         */
  /* ********************************* */

  /**
   * State used only in global writes, where the user can "append"
   * by successive query submissions until the query is finalized.
   */
  struct GlobalWriteState {
    /**
     * Stores the last tile of each attribute/dimension for each write
     * operation. For fixed-sized attributes/dimensions, the second tile is
     * ignored. For var-sized attributes/dimensions, the first tile is the
     * offsets tile, whereas the second tile is the values tile. In both cases,
     * the third tile stores a validity tile for nullable attributes.
     */
    std::unordered_map<std::string, std::tuple<Tile, Tile, Tile>> last_tiles_;

    /**
     * Stores the number of cells written for each attribute/dimension across
     * the write operations.
     */
    std::unordered_map<std::string, uint64_t> cells_written_;

    /** The fragment metadata. */
    tdb_shared_ptr<FragmentMetadata> frag_meta_;
  };

  /* ********************************* */
  /*     CONSTRUCTORS & DESTRUCTORS    */
  /* ********************************* */

  /** Constructor. */
  Writer(stats::Stats* parent_stats);

  /** Destructor. */
  ~Writer();

  DISABLE_COPY_AND_COPY_ASSIGN(Writer);
  DISABLE_MOVE_AND_MOVE_ASSIGN(Writer);

  /* ********************************* */
  /*                 API               */
  /* ********************************* */

  /** Returns the array. */
  const Array* array() const;

  /** Adds a range to the subarray on the input dimension. */
  Status add_range(unsigned dim_idx, Range&& range);

  /**
   * Disables checking the global order. Applicable only to writes.
   * This option will supercede the config.
   */
  void disable_check_global_order();

  /** Retrieves the number of ranges of the subarray for the given dimension. */
  Status get_range_num(unsigned dim_idx, uint64_t* range_num) const;

  /**
   * Retrieves a range from a dimension in the form (start, end, stride).
   *
   * @param dim_idx The dimension to retrieve the range from.
   * @param range_idx The id of the range to retrieve.
   * @param start The range start to retrieve.
   * @param end The range end to retrieve.
   * @param stride The range stride to retrieve.
   * @return Status
   */
  Status get_range(
      unsigned dim_idx,
      uint64_t range_idx,
      const void** start,
      const void** end,
      const void** stride) const;

  /** Returns the array schema. */
  const ArraySchema* array_schema() const;

  /** Returns the names of the buffers set by the user for the write query. */
  std::vector<std::string> buffer_names() const;

  /**
   * Returns the query buffer for the given attribute/dimension name.
   * The name can be TILEDB_COORDS.
   */
  QueryBuffer buffer(const std::string& name) const;

  /** Finalizes the reader. */
  Status finalize();

  /**
   * Retrieves the buffer of a fixed-sized attribute/dimension.
   *
   * @param name The buffer name.
   * @param buffer The buffer to be retrieved.
   * @param buffer_size A pointer to the buffer size to be retrieved.
   * @return Status
   */
  Status get_buffer(
      const std::string& name, void** buffer, uint64_t** buffer_size) const;

  /**
   * Retrieves the offsets and values buffers of a var-sized
   * attribute/dimension.
   *
   * @param name The buffer attribute/dimension.
   * @param buffer_off The offsets buffer to be retrieved.
   * @param buffer_off_size A pointer to the offsets buffer size to be
   * retrieved.
   * @param buffer_val The values buffer to be retrieved.
   * @param buffer_val_size A pointer to the values buffer size to be retrieved.
   * @return Status
   */
  Status get_buffer(
      const std::string& name,
      uint64_t** buffer_off,
      uint64_t** buffer_off_size,
      void** buffer_val,
      uint64_t** buffer_val_size) const;

  /**
   * Retrieves the buffer of a fixed-sized, nullable attribute.
   *
   * @param name The buffer name.
   * @param buffer The buffer to be retrieved.
   * @param buffer_size A pointer to the buffer size to be retrieved.
   * @param validity_vector A pointer to the validity vector to be retrieved.
   * @return Status
   */
  Status get_buffer_nullable(
      const std::string& name,
      void** buffer,
      uint64_t** buffer_size,
      const ValidityVector** validity_vector) const;

  /**
   * Retrieves the offsets and values buffers of a var-sized,
   * nullable attribute.
   *
   * @param name The buffer attribute/dimension.
   * @param buffer_off The offsets buffer to be retrieved.
   * @param buffer_off_size A pointer to the offsets buffer size to be
   * retrieved.
   * @param buffer_val The values buffer to be retrieved.
   * @param buffer_val_size A pointer to the values buffer size to be retrieved.
   * @param validity_vector A pointer to the validity vector to be retrieved.
   * @return Status
   */
  Status get_buffer_nullable(
      const std::string& name,
      uint64_t** buffer_off,
      uint64_t** buffer_off_size,
      void** buffer_val,
      uint64_t** buffer_val_size,
      const ValidityVector** validity_vector) const;

  /** Returns current setting of check_coord_dups_ */
  bool get_check_coord_dups() const;

  /** Returns current setting of check_coord_oob_ */
  bool get_check_coord_oob() const;

  /** Returns current setting of dedup_coords_ */
  bool get_dedup_coords() const;

  /** Returns the configured offsets format mode. */
  std::string get_offsets_mode() const;

  /** Returns `True` if offsets are configured to have an extra element. */
  bool get_offsets_extra_element() const;

  /** Returns the configured offsets bitsize */
  uint32_t get_offsets_bitsize() const;

  /** Initializes the writer with the subarray layout. */
  Status init(const Layout& layout);

  /** Returns the cell layout. */
  Layout layout() const;

  /** Sets the array. */
  void set_array(const Array* array);

  /*
   * Sets the array schema. If the array is a kv store, then this
   * function also sets global order as the default layout.
   */
  void set_array_schema(const ArraySchema* array_schema);

  /**
   * Sets the buffer for a fixed-sized attribute/dimension.
   *
   * @param name The attribute/dimension to set the buffer for.
   * @param buffer The buffer that has the input data to be written.
   * @param buffer_size The size of `buffer` in bytes.
   * @return Status
   */
  Status set_buffer(
      const std::string& name, void* buffer, uint64_t* buffer_size);

  /**
   * Sets the buffer for a var-sized attribute/dimension.
   *
   * @param name The attribute/dimension to set the buffer for.
   * @param buffer_off The buffer that has the input data to be written,
   *     This buffer holds the starting offsets of each cell value in
   *     `buffer_val`.
   * @param buffer_off_size The size of `buffer_off` in bytes.
   * @param buffer_val The buffer that has the input data to be written.
   *     This buffer holds the actual var-sized cell values.
   * @param buffer_val_size The size of `buffer_val` in bytes.
   * @return Status
   */
  Status set_buffer(
      const std::string& name,
      uint64_t* buffer_off,
      uint64_t* buffer_off_size,
      void* buffer_val,
      uint64_t* buffer_val_size);

  /**
   * Sets the buffer for a fixed-sized, nullable attribute.
   *
   * @param name The attribute to set the buffer for.
   * @param buffer The buffer that has the input data to be written.
   * @param buffer_size The size of `buffer` in bytes.
   * @param validity_vector The validity vector associated with values in
   * `buffer`.
   * @return Status
   */
  Status set_buffer(
      const std::string& name,
      void* buffer,
      uint64_t* buffer_size,
      ValidityVector&& validity_vector);

  /**
   * Sets the buffer for a var-sized, nullable attribute.
   *
   * @param name The attribute to set the buffer for.
   * @param buffer_off The buffer that has the input data to be written,
   *     This buffer holds the starting offsets of each cell value in
   *     `buffer_val`.
   * @param buffer_off_size The size of `buffer_off` in bytes.
   * @param buffer_val The buffer that has the input data to be written.
   *     This buffer holds the actual var-sized cell values.
   * @param buffer_val_size The size of `buffer_val` in bytes.
   * @param validity_vector The validity vector associated with values in
   * `buffer_val`.
   * @return Status
   */
  Status set_buffer(
      const std::string& name,
      uint64_t* buffer_off,
      uint64_t* buffer_off_size,
      void* buffer_val,
      uint64_t* buffer_val_size,
      ValidityVector&& validity_vector);

  /** Sets config for query-level parameters only. */
  Status set_config(const Config& config);

  /**
   * Get the config of the writer
   * @return Config
   */
  const Config* config() const;

  /** Sets current setting of check_coord_dups_ */
  void set_check_coord_dups(bool b);

  /** Sets current setting of check_coord_oob_ */
  void set_check_coord_oob(bool b);

  /** Sets current setting of dedup_coords_ */
  void set_dedup_coords(bool b);

  /** Sets the offsets format mode. */
  Status set_offsets_mode(const std::string& offsets_mode);

  /** Sets if offsets are configured to have an extra element. */
  Status set_offsets_extra_element(bool add_extra_element);

  /** Sets the bitsize of offsets */
  Status set_offsets_bitsize(const uint32_t bitsize);

  /** Sets the fragment URI. Applicable only to write queries. */
  void set_fragment_uri(const URI& fragment_uri);

  /**
   * Sets the cell layout of the query. The function will return an error
   * if the queried array is a key-value store (because it has its default
   * layout for both reads and writes.
   */
  Status set_layout(Layout layout);

  /** Sets the storage manager. */
  void set_storage_manager(StorageManager* storage_manager);

  /** Sets the query subarray. */
  Status set_subarray(const Subarray& subarray);

  /* Return the subarray. */
  const void* subarray() const;

  /** Returns the query subarray object. */
  const Subarray* subarray_ranges() const;

  /** Returns `stats_`. */
  stats::Stats* stats();

  /** Performs a write query using its set members. */
  Status write();

  /** Returns the written fragment info. */
  const std::vector<WrittenFragmentInfo>& written_fragment_info() const;

 private:
  /* ********************************* */
  /*         PRIVATE ATTRIBUTES        */
  /* ********************************* */

  /** The class stats. */
  stats::Stats* stats_;

  /** The array. */
  const Array* array_;

  /** The array schema. */
  const ArraySchema* array_schema_;

  /** The config for query-level parameters only. */
  Config config_;

  /** Maps attribute/dimensions names to their buffers. */
  std::unordered_map<std::string, QueryBuffer> buffers_;

  /** The coordinates buffer potentially set by the user. */
  void* coords_buffer_;

  /** The coordinates buffer size potentially set by the user. */
  uint64_t* coords_buffer_size_;

  /**
   * The sizes of the coordinate buffers in a map (dimension -> size).
   * Needed separate storage since QueryBuffer stores a pointer to the buffer
   * sizes.
   */
  std::unordered_map<std::string, uint64_t> coord_buffer_sizes_;

  /** True if at least one separate coordinate buffer is set. */
  bool coord_buffer_is_set_;

  /** Keeps track of the number of coordinates across coordinate buffers. */
  uint64_t coords_num_;

  /**
   * If `true`, it will not check if the written coordinates are
   * in the global order. This supercedes the config.
   */
  bool disable_check_global_order_;

  /**
   * True if either zipped coordinates buffer or separate coordinate
   * buffers are set.
   */
  bool has_coords_;

  /**
   * Meaningful only when `dedup_coords_` is `false`.
   * If `true`, a check for duplicate coordinates will be performed upon
   * sparse writes and appropriate errors will be thrown in case
   * duplicates are found.
   */
  bool check_coord_dups_;

  /**
   * If `true`, a check for coordinates lying out-of-bounds (i.e.,
   * outside the array domain) will be performed upon
   * sparse writes and appropriate errors will be thrown in case
   * such coordinates are found.
   */
  bool check_coord_oob_;

  /**
   * If `true`, the coordinates will be checked whether the
   * obey the global array order and appropriate errors will be thrown.
   */
  bool check_global_order_;

  /**
   * If `true`, deduplication of coordinates/cells will happen upon
   * sparse writes. Ties are broken arbitrarily.
   *
   */
  bool dedup_coords_;

  /** The name of the new fragment to be created. */
  URI fragment_uri_;

  /** The state associated with global writes. */
  tdb_unique_ptr<GlobalWriteState> global_write_state_;

  /** True if the writer has been initialized. */
  bool initialized_;

  /**
   * The layout of the cells in the result of the subarray. Note
   * that this may not be the same as what the user set to the
   * query, as the Writer may calibrate it to boost performance.
   */
  Layout layout_;

  /** The storage manager. */
  StorageManager* storage_manager_;

  /**
   * The subarray the query is constrained on. It is represented
   * as a flat byte vector for the (low, high) pairs of the
   * subarray. This is used only in dense writes and, therefore,
   * it is assumed that all dimensions have the same datatype.
   */
  std::vector<uint8_t> subarray_flat_;

  /**
   * The subarray object, used in dense writes. It has to be
   * comprised of a single multi-dimensional range.
   */
  Subarray subarray_;

  /** Stores information about the written fragments. */
  std::vector<WrittenFragmentInfo> written_fragment_info_;

  /** Allocated buffers that neeed to be cleaned upon destruction. */
  std::vector<void*> to_clean_;

  /** The offset format used for variable-sized attributes. */
  std::string offsets_format_mode_;

  /**
   * If `true`, an extra element that points to the end of the values buffer
   * will be added in the end of the offsets buffer of var-sized attributes
   */
  bool offsets_extra_element_;

  /** The offset bitsize used for variable-sized attributes. */
  uint32_t offsets_bitsize_;

  /* ********************************* */
  /*           PRIVATE METHODS         */
  /* ********************************* */

  /** Adss a fragment to `written_fragment_info_`. */
  Status add_written_fragment_info(const URI& uri);

  /** Checks if the buffers names have been appropriately set for the query. */
  Status check_buffer_names();

  /** Correctness checks for buffer sizes. */
  Status check_buffer_sizes() const;

  /**
   * Throws an error if there are coordinate duplicates.
   *
   * @param cell_pos The sorted positions of the coordinates in the
   *     `attr_buffers_`.
   * @return Status
   */
  Status check_coord_dups(const std::vector<uint64_t>& cell_pos) const;

  /**
   * Throws an error if there are coordinates falling out-of-bounds, i.e.,
   * outside the array domain.
   *
   * @return Status
   */
  Status check_coord_oob() const;

  /**
   * Throws an error if there are coordinate duplicates. This function
   * assumes that the coordinates are written in the global layout,
   * which means that they are already sorted in the attribute buffers.
   *
   * @return Status
   */
  Status check_coord_dups() const;

  /**
   * Throws an error if there are coordinates that do not obey the
   * global order.
   *
   * @return Status
   */
  Status check_global_order() const;

  /**
   * Throws an error if there are coordinates that do not obey the
   * global order. Applicable only to Hilbert order.
   *
   * @return Status
   */
  Status check_global_order_hilbert() const;

  /** Correctness checks for `subarray_`. */
  Status check_subarray() const;

  /**
   * Check the validity of the provided buffer offsets for a variable attribute.
   *
   * @return Status
   */
  Status check_var_attr_offsets() const;

  /**
   * Cleans up the coordinate buffers. Applicable only if the coordinate
   * buffers were allocated by TileDB (not the user)
   */
  void clear_coord_buffers();

  /** Closes all attribute files, flushing their state to storage. */
  Status close_files(FragmentMetadata* meta) const;

  /**
   * Computes the positions of the coordinate duplicates (if any). Note
   * that only the duplicate occurrences are determined, i.e., if the same
   * coordinates appear 3 times, only 2 will be marked as duplicates,
   * whereas the first occurrence will not be marked as duplicate.
   *
   * @param cell_pos The sorted positions of the coordinates in the
   *     `attr_buffers_`.
   * @param A set indicating the positions of the duplicates.
   *     If there are not duplicates, this vector will be **empty** after
   *     the termination of the function.
   * @return Status
   */
  Status compute_coord_dups(
      const std::vector<uint64_t>& cell_pos,
      std::set<uint64_t>* coord_dups) const;

  /**
   * Computes the positions of the coordinate duplicates (if any). Note
   * that only the duplicate occurrences are determined, i.e., if the same
   * coordinates appear 3 times, only 2 will be marked as duplicates,
   * whereas the first occurrence will not be marked as duplicate.
   *
   * This functions assumes that the coordinates are laid out in the
   * global order and, hence, they are sorted in the attribute buffers.
   *
   * @param A set indicating the positions of the duplicates.
   *     If there are not duplicates, this vector will be **empty** after
   *     the termination of the function.
   * @return Status
   */
  Status compute_coord_dups(std::set<uint64_t>* coord_dups) const;

  /**
   * Computes the coordinates metadata (e.g., MBRs).
   *
   * @param tiles The tiles to calculate the coords metadata from. It is
   *     a vector of vectors, one vector of tiles per dimension.
   * @param meta The fragment metadata that will store the coords metadata.
   * @return Status
   */
  Status compute_coords_metadata(
      const std::unordered_map<std::string, std::vector<Tile>>& tiles,
      FragmentMetadata* meta) const;

  /**
   * Creates a new fragment.
   *
   * @param dense Whether the fragment is dense or not.
   * @param frag_meta The fragment metadata to be generated.
   * @return Status
   */
  Status create_fragment(
      bool dense, tdb_shared_ptr<FragmentMetadata>* frag_meta) const;

  /**
   * Runs the input coordinate and attribute tiles through their
   * filter pipelines. The tile buffers are modified to contain the output
   * of the pipeline.
   */
  Status filter_tiles(
      std::unordered_map<std::string, std::vector<Tile>>* tiles);

  /**
   * Applicable only to global writes. Filters the last attribute and
   * coordinate tiles.
   */
  Status filter_last_tiles(
      std::unordered_map<std::string, std::vector<Tile>>* tiles);

  /**
   * Runs the input tiles for the input attribute through the filter pipeline.
   * The tile buffers are modified to contain the output of the pipeline.
   *
   * @param name The attribute/dimension the tiles belong to.
   * @param tile The tiles to be filtered.
   * @return Status
   */
  Status filter_tiles(const std::string& name, std::vector<Tile>* tiles);

  /**
   * Runs the input tile for the input attribute/dimension through the filter
   * pipeline. The tile buffer is modified to contain the output of the
   * pipeline.
   *
   * @param name The attribute/dimension the tile belong to.
   * @param tile The tile to be filtered.
   * @param offsets True if the tile to be filtered contains offsets for a
   *    var-sized attribute/dimension.
   * @param offsets True if the tile to be filtered contains validity values.
   * @return Status
   */
  Status filter_tile(
      const std::string& name, Tile* tile, bool offsets, bool nullable);

  /** Finalizes the global write state. */
  Status finalize_global_write_state();

  /**
   * Writes in the global layout. Applicable to both dense and sparse
   * arrays.
   */
  Status global_write();

  /**
   * Applicable only to global writes. Writes the last tiles for each
   * attribute remaining in the state, and records the metadata for
   * the coordinates (if present).
   *
   * @return Status
   */
  Status global_write_handle_last_tile();

  /** Initializes the global write state. */
  Status init_global_write_state();

  /**
   * Initializes a fixed-sized tile.
   *
   * @param name The attribute/dimension the tile belongs to.
   * @param tile The tile to be initialized.
   * @return Status
   */
  Status init_tile(const std::string& name, Tile* tile) const;

  /**
   * Initializes a var-sized tile.
   *
   * @param name The attribute/dimension the tile belongs to.
   * @param tile The offsets tile to be initialized.
   * @param tile_var The var-sized data tile to be initialized.
   * @return Status
   */
  Status init_tile(const std::string& name, Tile* tile, Tile* tile_var) const;

  /**
   * Initializes a fixed-sized, nullable tile.
   *
   * @param name The attribute the tile belongs to.
   * @param tile The tile to be initialized.
   * @param tile_validity The validity tile to be initialized.
   * @return Status
   */
  Status init_tile_nullable(
      const std::string& name, Tile* tile, Tile* tile_validity) const;

  /**
   * Initializes a var-sized, nullable tile.
   *
   * @param name The attribute the tile belongs to.
   * @param tile The offsets tile to be initialized.
   * @param tile_var The var-sized data tile to be initialized.
   * @param tile_validity The validity tile to be initialized.
   * @return Status
   */
  Status init_tile_nullable(
      const std::string& name,
      Tile* tile,
      Tile* tile_var,
      Tile* tile_validity) const;

  /**
   * Initializes the tiles for writing for the input attribute/dimension.
   *
   * @param name The attribute/dimension the tiles belong to.
   * @param tile_num The number of tiles.
   * @param tiles The tiles to be initialized. Note that the vector
   *     has been already preallocated.
   * @return Status
   */
  Status init_tiles(
      const std::string& name,
      uint64_t tile_num,
      std::vector<Tile>* tiles) const;

  /**
   * Generates a new fragment name, which is in the form: <br>
   * `__t_t_uuid_v`, where `t` is the input timestamp and `v` is the current
   * format version. For instance,
   * `__1458759561320_1458759561320_6ba7b8129dad11d180b400c04fd430c8_3`.
   *
   * If `timestamp` is 0, then it is set to the current time.
   *
   * @param timestamp The timestamp of when the array got opened for writes. It
   *     is in ms since 1970-01-01 00:00:00 +0000 (UTC).
   * @param frag_uri Will store the new special fragment name
   * @return Status
   */
  Status new_fragment_name(
      uint64_t timestamp, uint32_t format_version, std::string* frag_uri) const;

  /**
   * This deletes the global write state and deletes the potentially
   * partially written fragment.
   */
  void nuke_global_write_state();

  /**
   * Optimize the layout for 1D arrays. Specifically, if the array
   * is 1D and the query layout is not global or unordered, the layout
   * should be the same as the cell order of the array. This produces
   * equivalent results offering faster processing.
   */
  void optimize_layout_for_1D();

  /**
   * Checks the validity of the extra element from var-sized offsets of
   * attributes
   */
  Status check_extra_element();

  /**
   * Writes in an ordered layout (col- or row-major order). Applicable only
   * to dense arrays.
   */
  Status ordered_write();

  /**
   * Writes in an ordered layout (col- or row-major order). Applicable only
   * to dense arrays.
   *
   * @tparam T The domain type.
   */
  template <class T>
  Status ordered_write();

  /**
   * Return an element of the offsets buffer at a certain position
   * taking into account the configured bitsize
   */
  uint64_t get_offset_buffer_element(
      const void* buffer, const uint64_t pos) const;

  /**
   * Return the size of an offsets buffer according to the configured
   * options for variable-sized attributes
   */
  inline uint64_t get_offset_buffer_size(const uint64_t buffer_size) const;

  /**
   * Return a buffer offset according to the configured options for
   * variable-sized attributes (e.g. transform a byte offset to element offset)
   */
  uint64_t prepare_buffer_offset(
      const void* buffer, const uint64_t pos, const uint64_t datasize) const;

  /**
   * Applicable only to write in global order. It prepares only full
   * tiles, storing the last potentially non-full tile in
   * `global_write_state->last_tiles_` as part of the state to be used in
   * the next write invocation. The last tiles are written to storage
   * upon `finalize`. Upon each invocation, the function first
   * populates the partially full last tile from the previous
   * invocation.
   *
   * @param coord_dups The positions of the duplicate coordinates.
   * @param tiles The **full** tiles to be created.
   * @return Status
   */
  Status prepare_full_tiles(
      const std::set<uint64_t>& coord_dups,
      std::unordered_map<std::string, std::vector<Tile>>* tiles) const;

  /**
   * Applicable only to write in global order. It prepares only full
   * tiles, storing the last potentially non-full tile in
   * `global_write_state->last_tiles_` as part of the state to be used in
   * the next write invocation. The last tiles are written to storage
   * upon `finalize`. Upon each invocation, the function first
   * populates the partially full last tile from the previous
   * invocation.
   *
   * @param name The attribute/dimension to prepare the tiles for.
   * @param coord_dups The positions of the duplicate coordinates.
   * @param tiles The **full** tiles to be created.
   * @return Status
   */
  Status prepare_full_tiles(
      const std::string& name,
      const std::set<uint64_t>& coord_dups,
      std::vector<Tile>* tiles) const;

  /**
   * Applicable only to write in global order. It prepares only full
   * tiles, storing the last potentially non-full tile in
   * `global_write_state_->last_tiles_` as part of the state to be used in
   * the next write invocation. The last tiles are written to storage
   * upon `finalize`. Upon each invocation, the function first
   * populates the partially full last tile from the previous
   * invocation. Applicable only to fixed-sized attributes.
   *
   * @param name The attribute/dimension to prepare the tiles for.
   * @param coord_dups The positions of the duplicate coordinates.
   * @param tiles The **full** tiles to be created.
   * @return Status
   */
  Status prepare_full_tiles_fixed(
      const std::string& name,
      const std::set<uint64_t>& coord_dups,
      std::vector<Tile>* tiles) const;

  /**
   * Applicable only to write in global order. It prepares only full
   * tiles, storing the last potentially non-full tile in
   * `global_write_state_->last_tiles_` as part of the state to be used in
   * the next write invocation. The last tiles are written to storage
   * upon `finalize`. Upon each invocation, the function first
   * populates the partially full last tile from the previous
   * invocation. Applicable only to var-sized attributes.
   *
   * @param name The attribute/dimension to prepare the tiles for.
   * @param coord_dups The positions of the duplicate coordinates.
   * @param tiles The **full** tiles to be created.
   * @return Status
   */
  Status prepare_full_tiles_var(
      const std::string& name,
      const std::set<uint64_t>& coord_dups,
      std::vector<Tile>* tiles) const;

  /**
   * It prepares the attribute and coordinate tiles, re-organizing the cells
   * from the user buffers based on the input sorted positions and coordinate
   * duplicates.
   *
   * @param cell_pos The positions that resulted from sorting and
   *     according to which the cells must be re-arranged.
   * @param coord_dups The set with the positions
   *     of duplicate coordinates/cells.
   * @param tiles The tiles to be created, one vector per attribute or
   *     coordinate.
   * @return Status
   */
  Status prepare_tiles(
      const std::vector<uint64_t>& cell_pos,
      const std::set<uint64_t>& coord_dups,
      std::unordered_map<std::string, std::vector<Tile>>* tiles) const;

  /**
   * It prepares the tiles for the input attribute or dimension, re-organizing
   * the cells from the user buffers based on the input sorted positions.
   *
   * @param name The attribute or dimension to prepare the tiles for.
   * @param cell_pos The positions that resulted from sorting and
   *     according to which the cells must be re-arranged.
   * @param coord_dups The set with the positions
   *     of duplicate coordinates/cells.
   * @param tiles The tiles to be created.
   * @return Status
   */
  Status prepare_tiles(
      const std::string& name,
      const std::vector<uint64_t>& cell_pos,
      const std::set<uint64_t>& coord_dups,
      std::vector<Tile>* tiles) const;

  /**
   * It prepares the tiles for the input attribute or dimension, re-organizing
   * the cells from the user buffers based on the input sorted positions.
   * Applicable only to fixed-sized attributes or dimensions.
   *
   * @param name The attribute or dimension to prepare the tiles for.
   * @param cell_pos The positions that resulted from sorting and
   *     according to which the cells must be re-arranged.
   * @param coord_dups The set with the positions
   *     of duplicate coordinates/cells.
   * @param tiles The tiles to be created.
   * @return Status
   */
  Status prepare_tiles_fixed(
      const std::string& name,
      const std::vector<uint64_t>& cell_pos,
      const std::set<uint64_t>& coord_dups,
      std::vector<Tile>* tiles) const;

  /**
   * It prepares the tiles for the input attribute or dimension, re-organizing
   * the cells from the user buffers based on the input sorted positions.
   * Applicable only to var-sized attributes or dimensions.
   *
   * @param name The attribute to prepare the tiles for.
   * @param cell_pos The positions that resulted from sorting and
   *     according to which the cells must be re-arranged.
   * @param coord_dups The set with the positions
   *     of duplicate coordinates/cells.
   * @param tiles The tiles to be created.
   * @return Status
   */
  Status prepare_tiles_var(
      const std::string& name,
      const std::vector<uint64_t>& cell_pos,
      const std::set<uint64_t>& coord_dups,
      std::vector<Tile>* tiles) const;

  /** Resets the writer object, rendering it incomplete. */
  void reset();

  /**
   * Sorts the coordinates of the user buffers, creating a vector with
   * the sorted positions.
   *
   * @param cell_pos The sorted cell positions to be created.
   * @return Status
   */
  Status sort_coords(std::vector<uint64_t>* cell_pos) const;

  /**
   * Splits the coordinates buffer into separate coordinate
   * buffers, one per dimension. Note that this will require extra memory
   * allocation, which will be cleaned up in the class destructor.
   *
   * @return Status
   */
  Status split_coords_buffer();

  /**
   * Writes in unordered layout. Applicable to both dense and sparse arrays.
   * Explicit coordinates must be provided for this write.
   */
  Status unordered_write();

  /**
   * Writes an empty cell range to the input tile.
   * Applicable to **fixed-sized** attributes.
   *
   * @param cell_num Number of empty cells to write.
   * @param cell_val_num Number of values per cell.
   * @param tile The tile to write to.
   * @return Status
   */
  Status write_empty_cell_range_to_tile(
      uint64_t num, uint32_t cell_val_num, Tile* tile) const;

  /**
   * Writes an empty cell range to the input tile.
   * Applicable to **fixed-sized** attributes.
   *
   * @param cell_num Number of empty cells to write.
   * @param cell_val_num Number of values per cell.
   * @param tile The tile to write to.
   * @param tile_validity The tile with the validity cells to write to.
   * @return Status
   */
  Status write_empty_cell_range_to_tile_nullable(
      uint64_t num,
      uint32_t cell_val_num,
      Tile* tile,
      Tile* tile_validity) const;

  /**
   * Writes an empty cell range to the input tile.
   * Applicable to **variable-sized** attributes.
   *
   * @param num Number of empty values to write.
   * @param tile The tile offsets to write to.
   * @param tile_var The tile with the var-sized cells to write to.
   * @return Status
   */
  Status write_empty_cell_range_to_tile_var(
      uint64_t num, Tile* tile, Tile* tile_var) const;

  /**
   * Writes an empty cell range to the input tile.
   * Applicable to **variable-sized** attributes.
   *
   * @param num Number of empty values to write.
   * @param tile The tile offsets to write to.
   * @param tile_var The tile with the var-sized cells to write to.
   * @param tile_validity The tile with the validity cells to write to.
   * @return Status
   */
  Status write_empty_cell_range_to_tile_var_nullable(
      uint64_t num, Tile* tile, Tile* tile_var, Tile* tile_validity) const;

  /**
   * Writes the input cell range to the input tile, for a particular
   * buffer. Applicable to **fixed-sized** attributes.
   *
   * @param buff The write buffer where the cells will be copied from.
   * @param start The start element in the write buffer.
   * @param end The end element in the write buffer.
   * @param tile The tile to write to.
   * @return Status
   */
  Status write_cell_range_to_tile(
      ConstBuffer* buff, uint64_t start, uint64_t end, Tile* tile) const;

  /**
   * Writes the input cell range to the input tile, for a particular
   * buffer. Applicable to **fixed-sized** attributes.
   *
   * @param buff The write buffer where the cells will be copied from.
   * @param buff_validity The write buffer where the validity cell values will
   * be copied from.
   * @param start The start element in the write buffer.
   * @param end The end element in the write buffer.
   * @param tile The tile to write to.
   * @param tile_validity The validity tile to be initialized.
   * @return Status
   */
  Status write_cell_range_to_tile_nullable(
      ConstBuffer* buff,
      ConstBuffer* buff_validity,
      uint64_t start,
      uint64_t end,
      Tile* tile,
      Tile* tile_validity) const;

  /**
   * Writes the input cell range to the input tile, for a particular
   * buffer. Applicable to **variable-sized** attributes.
   *
   * @param buff The write buffer where the cell offsets will be copied from.
   * @param buff_var The write buffer where the cell values will be copied from.
   * @param start The start element in the write buffer.
   * @param end The end element in the write buffer.
   * @param attr_datatype_size The size of each attribute value in `buff_var`.
   * @param tile The tile offsets to write to.
   * @param tile_var The tile with the var-sized cells to write to.
   * @return Status
   */
  Status write_cell_range_to_tile_var(
      ConstBuffer* buff,
      ConstBuffer* buff_var,
      uint64_t start,
      uint64_t end,
      uint64_t attr_datatype_size,
      Tile* tile,
      Tile* tile_var) const;

  /**
   * Writes the input cell range to the input tile, for a particular
   * buffer. Applicable to **variable-sized**, nullable attributes.
   *
   * @param buff The write buffer where the cell offsets will be copied from.
   * @param buff_var The write buffer where the cell values will be copied from.
   * @param buff_validity The write buffer where the validity cell values will
   * be copied from.
   * @param start The start element in the write buffer.
   * @param end The end element in the write buffer.
   * @param attr_datatype_size The size of each attribute value in `buff_var`.
   * @param tile The tile offsets to write to.
   * @param tile_var The tile with the var-sized cells to write to.
   * @param tile_validity The validity tile to be initialized.
   * @return Status
   */
  Status write_cell_range_to_tile_var_nullable(
      ConstBuffer* buff,
      ConstBuffer* buff_var,
      ConstBuffer* buff_validity,
      uint64_t start,
      uint64_t end,
      uint64_t attr_datatype_size,
      Tile* tile,
      Tile* tile_var,
      Tile* tile_validity) const;

  /**
   * Writes all the input tiles to storage.
   *
   * @param tiles Attribute/Coordinate tiles to be written, one element per
   *     attribute or dimension.
   * @param tiles Attribute/Coordinate tiles to be written.
   * @return Status
   */
  Status write_all_tiles(
      FragmentMetadata* frag_meta,
      std::unordered_map<std::string, std::vector<Tile>>* tiles);

  /**
   * Writes the input tiles for the input attribute/dimension to storage.
   *
   * @param name The attribute/dimension the tiles belong to.
   * @param frag_meta The fragment metadata.
   * @param start_tile_id The function will start writing tiles
   *     with ids in the fragment that start with this value.
   * @param tiles The tiles to be written.
   * @param close_files Whether to close the attribute/coordinate
   *     file in the end of the function call.
   * @return Status
   */
  Status write_tiles(
      const std::string& name,
      FragmentMetadata* frag_meta,
      uint64_t start_tile_id,
      std::vector<Tile>* tiles,
      bool close_files = true);

  /**
   * Returns the i-th coordinates in the coordinate buffers in string
   * format.
   */
  std::string coords_to_str(uint64_t i) const;

  /**
   * Invoked on error. It removes the directory of the input URI and
   * resets the global write state.
   */
  void clean_up(const URI& uri);

  /**
   * Applicable only to global writes. Returns true if all last tiles stored
   * in the global write state are empty.
   */
  bool all_last_tiles_empty() const;

  /**
   * Sets the (zipped) coordinates buffer (set with TILEDB_COORDS as the
   * buffer name).
   *
   * @param buffer The buffer that has the input data to be written.
   * @param buffer_size The size of `buffer` in bytes.
   * @return Status
   */
  Status set_coords_buffer(void* buffer, uint64_t* buffer_size);

  /** Gets statistics about dimensions and attributes written. */
  void get_dim_attr_stats() const;

  /** Calculates the hilbert values of the input coordinate buffers. */
  Status calculate_hilbert_values(
      const std::vector<const QueryBuffer*>& buffs,
      std::vector<uint64_t>* hilbert_values) const;

  /**
   * Prepares, filters and writes dense tiles for the given attribute.
   *
   * @tparam T The array domain datatype.
   * @param name The attribute name.
   * @param frag_meta The metadata of the new fragment.
   * @param dense_tiler The dense tiler that will prepare the tiles.
   * @param thread_num The number of threads to be used for the function.
   * @param stats Statistics to gather in the function.
   */
  template <class T>
  Status prepare_filter_and_write_tiles(
      const std::string& name,
      FragmentMetadata* frag_meta,
      DenseTiler<T>* dense_tiler,
      uint64_t thread_num);
};

}  // namespace sm
}  // namespace tiledb

#endif  // TILEDB_WRITER_H
