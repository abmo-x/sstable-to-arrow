#ifndef CONVERSION_HELPER_H_
#define CONVERSION_HELPER_H_

#include <arrow/api.h>
#include "conversions.h"
#include "sstable_statistics.h"

class column_t
{
public:
    using ts_builder_t = arrow::TimestampBuilder;
    using local_del_time_builder_t = arrow::TimestampBuilder;
    using ttl_builder_t = arrow::DurationBuilder;

    std::string cassandra_type;
    std::shared_ptr<arrow::Field> field;
    std::unique_ptr<arrow::ArrayBuilder> builder;
    std::unique_ptr<arrow::ArrayBuilder> ts_builder;
    std::unique_ptr<arrow::ArrayBuilder> local_del_time_builder;
    std::unique_ptr<arrow::ArrayBuilder> ttl_builder;

    // this constructor infers the arrow::DataType from the Cassandra type
    column_t(
        const std::string &name_,
        const std::string &cassandra_type_)
        : column_t(name_, cassandra_type_, conversions::get_arrow_type(cassandra_type_)) {}

    column_t(
        const std::string &name_,
        const std::string &cassandra_type_,
        std::shared_ptr<arrow::DataType> type_)
        : cassandra_type(cassandra_type_), field(arrow::field(name_, type_)){};

    arrow::Status init(arrow::MemoryPool *pool, bool complex_ts_allowed = true);

    // allocate enough memory for nrows elements in both the value and timestamp
    // builders
    arrow::Status reserve(uint32_t nrows);
    arrow::Status finish(std::shared_ptr<arrow::Array> *ptr);
    arrow::Status append_null();
};

class conversion_helper_t
{
public:
    conversion_helper_t(std::shared_ptr<sstable_statistics_t> statistics);

    std::shared_ptr<column_t> partition_key;
    std::vector<std::shared_ptr<column_t>> clustering_cols;
    std::vector<std::shared_ptr<column_t>> static_cols;
    std::vector<std::shared_ptr<column_t>> regular_cols;

    // metadata from the Statistics.db file
    sstable_statistics_t::serialization_header_t *metadata;
    sstable_statistics_t::statistics_t *statistics;

    // get the actual timestamp based on the epoch, the minimum timestamp in
    // this SSTable, and the given delta
    arrow::Status init(arrow::MemoryPool *pool);

    uint64_t get_timestamp(uint64_t delta) const;
    uint64_t get_local_del_time(uint64_t delta) const;
    uint64_t get_ttl(uint64_t delta) const;

    size_t num_data_cols() const;
    size_t num_ts_cols() const;
    arrow::Status reserve();
    std::shared_ptr<arrow::Schema> schema() const;
    arrow::Result<std::shared_ptr<arrow::Table>> to_table() const;
};

// Recursively allocate memory for `nrows` elements in `builder` and its child
// builders.
arrow::Status reserve_builder(arrow::ArrayBuilder *builder, const int64_t &nrows);

// extract the serialization header from an SSTable
sstable_statistics_t::serialization_header_t *get_serialization_header(std::shared_ptr<sstable_statistics_t> statistics);

#endif