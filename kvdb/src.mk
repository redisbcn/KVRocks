# These are the sources from which librocksdb.a is built:
LIB_SOURCES =                                                   \
  db/builder.cc                                                   \
  db/column_family.cc                                                   \
  db/compaction.cc                                                   \
  db/compaction_iterator.cc                                                   \
  db/compaction_picker_universal.cc                                                     \
  db/compaction_picker.cc                                                     \
  db/convenience.cc \
  db/dbformat.cc                                                   \
  db/db_impl.cc                                                   \
  db/db_iter.cc                                                   \
  db/event_helpers.cc                                                   \
  db/file_indexer.cc                                                   \
  db/insdb_wrapper.cc                                                   \
  db/internal_stats.cc                                                   \
  db/log_reader.cc                                                   \
  db/log_writer.cc                                                   \
  db/memtable.cc                                                   \
  db/memtable_list.cc                                                   \
  db/merge_helper.cc                                                   \
  db/merge_operator.cc                                                   \
  db/range_del_aggregator.cc                                                   \
  db/table_cache.cc                                                   \
  db/table_properties_collector.cc                                                   \
  db/version_builder.cc                                                   \
  db/version_edit.cc                                                   \
  db/version_set.cc                                                   \
  db/write_batch_base.cc                                                   \
  db/write_batch.cc                                                   \
  db/write_controller.cc                                                   \
  cache/clock_cache.cc                                                   \
  cache/lru_cache.cc                                                   \
  env/env.cc                                                   \
  env/env_posix.cc                                                   \
  env/io_posix.cc                                                   \
  memtable/alloc_tracker.cc                                                   \
  memtable/hash_cuckoo_rep.cc                                                   \
  memtable/hash_linklist_rep.cc                                                   \
  memtable/hash_skiplist_rep.cc                                                   \
  memtable/skiplistrep.cc                                                   \
  memtable/vectorrep.cc                                                   \
  memtable/write_buffer_manager.cc                                                   \
  monitoring/histogram.cc                                                   \
  monitoring/instrumented_mutex.cc                                                   \
  monitoring/iostats_context.cc                                                   \
  monitoring/perf_context.cc                                                   \
  monitoring/perf_level.cc                                                   \
  monitoring/statistics.cc                                                   \
  monitoring/thread_status_impl.cc                                                   \
  monitoring/thread_status_updater.cc                                                   \
  monitoring/thread_status_util_debug.cc \
  monitoring/thread_status_util.cc                                                   \
  port/port_posix.cc                                                   \
  port/stack_trace.cc                                                   \
  table/block_based_filter_block.cc                                                   \
  table/block_based_table_builder.cc                                                   \
  table/block_based_table_factory.cc                                                   \
  table/block_based_table_reader.cc                                                   \
  table/block_builder.cc                                                   \
  table/block_prefix_index.cc                                                   \
  table/block.cc                                                   \
  table/bloom_block.cc                                                   \
  table/flush_block_policy.cc                                                   \
  table/format.cc                                                   \
  table/full_filter_block.cc                                                   \
  table/get_context.cc                                                   \
  table/index_builder.cc                                                      \
  table/iterator.cc                                                   \
  table/merging_iterator.cc                                                   \
  table/meta_blocks.cc                                                   \
  table/partitioned_filter_block.cc                                                   \
  table/persistent_cache_helper.cc                                                   \
  table/plain_table_builder.cc                                                   \
  table/plain_table_factory.cc                                                   \
  table/plain_table_index.cc                                                   \
  table/plain_table_key_coding.cc                                                   \
  table/plain_table_reader.cc                                                   \
  table/sst_file_writer.cc                                                   \
  table/table_properties.cc                                                   \
  table/two_level_iterator.cc                                                   \
  options/cf_options.cc                                                   \
  options/db_options.cc                                                   \
  options/options_helper.cc                                                   \
  options/options_parser.cc                                                   \
  options/options_sanity_check.cc                                                   \
  options/options.cc                                                   \
  util/arena.cc                                                   \
  util/auto_roll_logger.cc                                                   \
  util/bloom.cc                                                   \
  util/coding.cc                                                   \
  util/comparator.cc                                                   \
  util/concurrent_arena.cc                                                   \
  util/crc32c.cc                                                   \
  util/dynamic_bloom.cc                                                   \
  util/event_logger.cc                                                   \
  util/filename.cc                                                   \
  util/file_reader_writer.cc                                                   \
  util/filter_policy.cc                                                   \
  util/hash.cc                                                   \
  util/log_buffer.cc                                                   \
  util/murmurhash.cc                                                   \
  util/random.cc                                                   \
  util/rate_limiter.cc                                                   \
  util/slice.cc                                                   \
  util/sst_file_manager_impl.cc                                                   \
  util/status.cc                                                   \
  util/status_message.cc                                                   \
  util/string_util.cc                                                   \
  util/sync_point.cc                                                   \
  util/thread_local.cc                                                   \
  util/threadpool_imp.cc                                                   \
  util/xxhash.cc                                                   \
  utilities/checkpoint/checkpoint_impl.cc                               \
  utilities/memory/memory_util.cc                               \
  utilities/options/options_util.cc \
  utilities/persistent_cache/block_cache_tier.cc \
  utilities/simulator_cache/sim_cache.cc \
  utilities/transactions/pessimistic_transaction_db.cc \
  utilities/transactions/pessimistic_transaction.cc \
  utilities/transactions/transaction_base.cc \
  utilities/transactions/transaction_db_mutex_impl.cc \
  utilities/transactions/transaction_lock_mgr.cc \
  utilities/transactions/transaction_util.cc \
  utilities/transactions/write_prepared_txn.cc \
  utilities/write_batch_with_index/write_batch_with_index_internal.cc \
  utilities/write_batch_with_index/write_batch_with_index.cc \
  utilities/env_mirror.cc                                                   \
  
