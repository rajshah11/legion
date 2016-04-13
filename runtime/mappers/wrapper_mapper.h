#ifndef __WRAPPER_MAPPER_h__
#define __WRAPPER_MAPPER_h__

#include "default_mapper.h"

#include <cstdlib>
#include <cassert>
#include <algorithm>



namespace LegionRuntime {
	namespace HighLevel {

		class WrapperMapper: public Mapper{

		public:
			Mapper* dmapper;
			static std::vector<int> tasks_list;
			static std::map<std::string, int> tasks_map;
			static std::vector<int> functions_list;
			static std::map<int, int> methods_map;
			static std::set<Memory> all_mems;
			static std::set<Processor> all_procs;
			static std::vector<Memory> mems_list;
			static std::vector<Processor> procs_list;
			static std::map<Processor, int> procs_map;
			static std::map<Memory, int> mems_map;
			static std::map<int, std::string> task_names_map;
			static bool inputtaken;
			//static const char *namestring;
			WrapperMapper(Mapper* dmapper, Machine machine, HighLevelRuntime *rt, Processor local);
			~WrapperMapper(void);

		public:
			void select_task_options(Task *task);
			void select_tasks_to_schedule(
			const std::list<Task*> &ready_tasks);
			void target_task_steal(
			const std::set<Processor> &blacklist,
			std::set<Processor> &targets);
			void permit_task_steal(Processor thief, 
			const std::vector<const Task*> &tasks,
			std::set<const Task*> &to_steal);
			void slice_domain(const Task *task, const Domain &domain,
			std::vector<DomainSplit> &slices);
			bool pre_map_task(Task *task);
			void select_task_variant(Task *task);
			bool map_task(Task *task);
			void post_map_task(Task *task);
			bool map_copy(Copy *copy);
			bool map_inline(Inline *inline_operation);
			bool map_must_epoch(const std::vector<Task*> &tasks,
			const std::vector<MappingConstraint> &constraints,
			MappingTagID tag);
			void notify_mapping_result(const Mappable *mappable);
			void notify_mapping_failed(const Mappable *mappable);
			bool rank_copy_targets(const Mappable *mappable,
			LogicalRegion rebuild_region,
			const std::set<Memory> &current_instances,
			bool complete,
			size_t max_blocking_factor,
			std::set<Memory> &to_reuse,
			std::vector<Memory> &to_create,
			bool &create_one,
			size_t &blocking_factor);
			void rank_copy_sources(const Mappable *mappable,
			const std::set<Memory> &current_instances,
			Memory dst_mem, 
			std::vector<Memory> &chosen_order);
			void notify_profiling_info(const Task *task);
			bool speculate_on_predicate(const Mappable *mappable,
			bool &spec_value);
			void configure_context(Task *task);
			int get_tunable_value(const Task *task, 
			TunableID tid,
			MappingTagID tag);
			void handle_message(Processor source,
			const void *message,
			size_t length);
			void handle_mapper_task_result(MapperEvent event,
			const void *result,
			size_t result_size);
			bool InputMatches(std::string strUserInput, std::string strTemplate);
			bool InputNumberCheck(std::string strUserInput);
			void get_input();
			void get_select_task_options_input(Task *task);
			void get_map_task_input(Task *task);

		protected:
			const Processor local_proc;
			const Processor::Kind local_kind;
			const Machine machine;
			// The maximum number of tasks a mapper will allow to be stolen at a time
			// Controlled by -dm:thefts
			unsigned max_steals_per_theft;
			// The maximum number of times that a single task is allowed to be stolen
			// Controlled by -dm:count
			unsigned max_steal_count;
			// The splitting factor for breaking index spaces across the machine
			// Mapper will try to break the space into split_factor * num_procs
			// difference pieces
			// Controlled by -dm:split
			unsigned splitting_factor;
			// Do a breadth-first traversal of the task tree, by default we do
			// a depth-first traversal to improve locality
			bool breadth_first_traversal;
			// Whether or not copies can be made to avoid Write-After-Read dependences
			// Controlled by -dm:war
			bool war_enabled;
			// Track whether stealing is enabled
			bool stealing_enabled;
			// The maximum number of tasks scheduled per step
			unsigned max_schedule_count;
			// Maximum number of failed mappings for a task before error
			unsigned max_failed_mappings;
			std::map<UniqueID,unsigned> failed_mappings;
			
			// Utilities for use within the default mapper 
			MappingUtilities::MachineQueryInterface machine_interface;
			MappingUtilities::MappingMemoizer memoizer;
			MappingUtilities::MappingProfiler profiler;
		};
	};
};

#endif // __DEFAULT_MAPPER_H__

// EOF


