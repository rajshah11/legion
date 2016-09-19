/* Copyright 2016 Stanford University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <cstdio>
#include <cassert>
#include <cstdlib>
#include "legion.h"

#include "wrapper_mapper.h"
#include "default_mapper.h"

using namespace Legion;
using namespace LegionRuntime::Accessor;
using namespace Legion::Mapping;

/*
 * In this example, we perform the same
 * computation as the daxpy example. 
 * However, we wrap the mapper using 
 * two different mapper synchronization models, 
 * namely, serialized and concurrent. 
 * These mapper managers  define whether mapper 
 * calls are executed in a serializing or a 
 * concurrent manner. To do so, we implement a
 * custom mapper by just overriding the method
 * get_mapper_sync_model() of the mapping 
 * interface. This example also introduces a 
 * wrapper mapper, which can be used to monitor 
 * tasks and processors when important properties 
 * of a task are set by select_task_options().
 */

using namespace LegionRuntime::Arrays;

enum TaskIDs {
  TOP_LEVEL_TASK_ID,
  INIT_FIELD_TASK_ID,
  DAXPY_TASK_ID,
  CHECK_TASK_ID,
};

enum FieldIDs {
  FID_X,
  FID_Y,
  FID_Z,
};

/*
 * As you may recall, we want to allow
 * programmers to make application- or
 * architecture-specific mapping decisions.
 * One such effort in this regard is to
 * provide the programmers the option
 * to choose different mapper synchronization
 * models. The concurrent model allows multiple
 * mapper calls to be run at the same time.
 * The serializing model as the name suggests,
 * performs mapper calls in a serializing manner.
 * However, the mapper calls in the serializing 
 * model can be reentrant to perform utility 
 * mapper calls while pausing the mapper context. 
 * Thus, the serializing mapper model has two modes -
 * one when reentrant mapper calls are allowed
 * by default and other when reentrant calls are
 * not allowed.
 *
 * The wrapper mapper on, the other hand, is
 * just a custom mapper, which takes
 * a mapper as a parameter and wraps it to
 * provide basic debugging functionality when
 * properties for a task are set.
 *
 * In this example, based on the user input
 * the mapper synchronization model will be
 * selected by overriding get_mapper_sync_model().
 * Subsequently, we wrap the mapper using the
 * WrapperMapper class when the wrapper flag is
 * set to 1. 
 */

class CustomSyncMapper : public DefaultMapper {
  public:
    CustomSyncMapper(Machine machine, 
	Runtime *rt, Processor local);
  public:
    //Mapper call that returns the mapper synchronization model
    //The mapper call defined below returns a sync model based 
    //on the user input
    virtual MapperSyncModel get_mapper_sync_model(void) const;
};

//Constructor for CustomSyncMapper
CustomSyncMapper::CustomSyncMapper(Machine m, 
    Runtime *rt, Processor p)
: DefaultMapper(rt->get_mapper_runtime(), m, p)
{
}

//The get_mapper_sync_model() call must return any of the three
//MapperSyncModel values -
//SERIALIZED_REENTRANT_MAPPER_MODEL     -   Serializing sync model allowing
//				            reentrant calls by default
//SERIALIZED_NON_REENTRANT_MAPPER_MODEL -   Serializing sync model not allowing
//				            reentrant calls by default
//CONCURRENT_MAPPER_MODEL		-   Concurrent sync model
Mapper::MapperSyncModel CustomSyncMapper::get_mapper_sync_model(void) const
{
  //Check for user input
  int sync_model;
  const InputArgs &command_args = Runtime::get_input_args();
  for (int i = 1; i < command_args.argc; i++)
  {
    if (!strcmp(command_args.argv[i],"-s"))
      sync_model = atoi(command_args.argv[++i]);
  }

  switch (sync_model)
  {
    case 1:
      return SERIALIZED_REENTRANT_MAPPER_MODEL;

    case 2:
      return SERIALIZED_NON_REENTRANT_MAPPER_MODEL;  

    case 3:
      return CONCURRENT_MAPPER_MODEL;

    default:
      return SERIALIZED_REENTRANT_MAPPER_MODEL;
  }
}

//As in the custom mappers example, we now define the
//mapper_registration function to replace the DefaultMapper
//with our custom mapper. However, here we also wrap the 
//mapper using the WrapperMapper class if the user sets the
//-w flag to 1. The wrapped mapper results in a command
//line interface which the user can use to monitor tasks.
//The required commands to do so can be found by entering
//"help".
void mapper_registration(Machine machine, Runtime *rt,
    const std::set<Processor> &local_procs)
{
  //Check for user input
  int to_wrap = 0;
  const InputArgs &command_args = Runtime::get_input_args();
  for (int i = 1; i < command_args.argc; i++)
  {
    if (!strcmp(command_args.argv[i],"-w"))
      to_wrap = atoi(command_args.argv[++i]);
  }

  if (!to_wrap)
  {
    for (std::set<Processor>::const_iterator it = local_procs.begin();
	it != local_procs.end(); it++)
    {
      rt->replace_default_mapper(new CustomSyncMapper(machine, rt, *it));
    }
  }
  else
  {

    for (std::set<Processor>::const_iterator it = local_procs.begin();
	it != local_procs.end(); it++)
    {
      //One can wrap individual mappers using the constructor
      //Legion::Mapping::WrapperMapper
      //(Mapper* mapper, MapperRuntime *rt, Machine machine, Processor local);
      rt->replace_default_mapper(new WrapperMapper(
	    new CustomSyncMapper(machine, rt, *it), 
	    rt->get_mapper_runtime(), machine, *it), *it);
    }  
  }
}

void top_level_task(const Task *task,
    const std::vector<PhysicalRegion> &regions,
    Context ctx, Runtime *runtime)
{
  int num_elements = 1024; 
  int num_subregions = 4;
  {
    const InputArgs &command_args = Runtime::get_input_args();
    for (int i = 1; i < command_args.argc; i++)
    {
      if (!strcmp(command_args.argv[i],"-n"))
	num_elements = atoi(command_args.argv[++i]);
      if (!strcmp(command_args.argv[i],"-b"))
	num_subregions = atoi(command_args.argv[++i]);
    }
  }
  printf("Running daxpy for %d elements...\n", num_elements);
  printf("Partitioning data into %d sub-regions...\n", num_subregions);

  Rect<1> elem_rect(Point<1>(0),Point<1>(num_elements-1));
  IndexSpace is = runtime->create_index_space(ctx, 
      Domain::from_rect<1>(elem_rect));
  runtime->attach_name(is, "is");
  FieldSpace input_fs = runtime->create_field_space(ctx);
  runtime->attach_name(input_fs, "input_fs");
  {
    FieldAllocator allocator = 
      runtime->create_field_allocator(ctx, input_fs);
    allocator.allocate_field(sizeof(double),FID_X);
    runtime->attach_name(input_fs, FID_X, "X");
    allocator.allocate_field(sizeof(double),FID_Y);
    runtime->attach_name(input_fs, FID_Y, "Y");
  }
  FieldSpace output_fs = runtime->create_field_space(ctx);
  runtime->attach_name(output_fs, "output_fs");
  {
    FieldAllocator allocator = 
      runtime->create_field_allocator(ctx, output_fs);
    allocator.allocate_field(sizeof(double),FID_Z);
    runtime->attach_name(output_fs, FID_Z, "Z");
  }
  LogicalRegion input_lr = runtime->create_logical_region(ctx, is, input_fs);
  runtime->attach_name(input_lr, "input_lr");
  LogicalRegion output_lr = runtime->create_logical_region(ctx, is, output_fs);
  runtime->attach_name(output_lr, "output_lr");

  Rect<1> color_bounds(Point<1>(0),Point<1>(num_subregions-1));
  Domain color_domain = Domain::from_rect<1>(color_bounds);

  IndexPartition ip;
  if ((num_elements % num_subregions) != 0)
  {
    const int lower_bound = num_elements/num_subregions;
    const int upper_bound = lower_bound+1;
    const int number_small = num_subregions - (num_elements % num_subregions);

    DomainColoring coloring;
    int index = 0;
    for (int color = 0; color < num_subregions; color++)
    {
      int num_elmts = color < number_small ? lower_bound : upper_bound;
      assert((index+num_elmts) <= num_elements);
      Rect<1> subrect(Point<1>(index),Point<1>(index+num_elmts-1));
      coloring[color] = Domain::from_rect<1>(subrect);
      index += num_elmts;
    }
    ip = runtime->create_index_partition(ctx, is, color_domain, 
	coloring, true/*disjoint*/);
  }
  else
  { 
    Blockify<1> coloring(num_elements/num_subregions);
    ip = runtime->create_index_partition(ctx, is, coloring);
  }
  runtime->attach_name(ip, "ip");

  LogicalPartition input_lp = runtime->get_logical_partition(ctx, input_lr, ip);
  runtime->attach_name(input_lp, "input_lp");
  LogicalPartition output_lp = runtime->get_logical_partition(ctx, output_lr, ip);
  runtime->attach_name(output_lp, "output_lp");

  Domain launch_domain = color_domain; 
  ArgumentMap arg_map;

  IndexLauncher init_launcher(INIT_FIELD_TASK_ID, launch_domain, 
      TaskArgument(NULL, 0), arg_map);

  init_launcher.add_region_requirement(
      RegionRequirement(input_lp, 0/*projection ID*/, 
	WRITE_DISCARD, EXCLUSIVE, input_lr));
  init_launcher.region_requirements[0].add_field(FID_X);
  runtime->execute_index_space(ctx, init_launcher);

  init_launcher.region_requirements[0].privilege_fields.clear();
  init_launcher.region_requirements[0].instance_fields.clear();
  init_launcher.region_requirements[0].add_field(FID_Y);
  runtime->execute_index_space(ctx, init_launcher);

  const double alpha = drand48();
  IndexLauncher daxpy_launcher(DAXPY_TASK_ID, launch_domain,
      TaskArgument(&alpha, sizeof(alpha)), arg_map);
  daxpy_launcher.add_region_requirement(
      RegionRequirement(input_lp, 0/*projection ID*/,
	READ_ONLY, EXCLUSIVE, input_lr));
  daxpy_launcher.region_requirements[0].add_field(FID_X);
  daxpy_launcher.region_requirements[0].add_field(FID_Y);
  daxpy_launcher.add_region_requirement(
      RegionRequirement(output_lp, 0/*projection ID*/,
	WRITE_DISCARD, EXCLUSIVE, output_lr));
  daxpy_launcher.region_requirements[1].add_field(FID_Z);
  runtime->execute_index_space(ctx, daxpy_launcher);

  TaskLauncher check_launcher(CHECK_TASK_ID, TaskArgument(&alpha, sizeof(alpha)));
  check_launcher.add_region_requirement(
      RegionRequirement(input_lr, READ_ONLY, EXCLUSIVE, input_lr));
  check_launcher.region_requirements[0].add_field(FID_X);
  check_launcher.region_requirements[0].add_field(FID_Y);
  check_launcher.add_region_requirement(
      RegionRequirement(output_lr, READ_ONLY, EXCLUSIVE, output_lr));
  check_launcher.region_requirements[1].add_field(FID_Z);
  runtime->execute_task(ctx, check_launcher);

  runtime->destroy_logical_region(ctx, input_lr);
  runtime->destroy_logical_region(ctx, output_lr);
  runtime->destroy_field_space(ctx, input_fs);
  runtime->destroy_field_space(ctx, output_fs);
  runtime->destroy_index_space(ctx, is);
}

void init_field_task(const Task *task,
    const std::vector<PhysicalRegion> &regions,
    Context ctx, Runtime *runtime)
{
  assert(regions.size() == 1); 
  assert(task->regions.size() == 1);
  assert(task->regions[0].privilege_fields.size() == 1);

  FieldID fid = *(task->regions[0].privilege_fields.begin());
  const int point = task->index_point.point_data[0];
  printf("Initializing field %d for block %d...\n", fid, point);

  RegionAccessor<AccessorType::Generic, double> acc = 
    regions[0].get_field_accessor(fid).typeify<double>();

  Domain dom = runtime->get_index_space_domain(ctx, 
      task->regions[0].region.get_index_space());
  Rect<1> rect = dom.get_rect<1>();
  for (GenericPointInRectIterator<1> pir(rect); pir; pir++)
  {
    acc.write(DomainPoint::from_point<1>(pir.p), drand48());
  }
}

void daxpy_task(const Task *task,
    const std::vector<PhysicalRegion> &regions,
    Context ctx, Runtime *runtime)
{
  assert(regions.size() == 2);
  assert(task->regions.size() == 2);
  assert(task->arglen == sizeof(double));
  const double alpha = *((const double*)task->args);
  const int point = task->index_point.point_data[0];

  RegionAccessor<AccessorType::Generic, double> acc_x = 
    regions[0].get_field_accessor(FID_X).typeify<double>();
  RegionAccessor<AccessorType::Generic, double> acc_y = 
    regions[0].get_field_accessor(FID_Y).typeify<double>();
  RegionAccessor<AccessorType::Generic, double> acc_z = 
    regions[1].get_field_accessor(FID_Z).typeify<double>();
  printf("Running daxpy computation with alpha %.8g for point %d...\n", 
      alpha, point);

  Domain dom = runtime->get_index_space_domain(ctx, 
      task->regions[0].region.get_index_space());
  Rect<1> rect = dom.get_rect<1>();
  for (GenericPointInRectIterator<1> pir(rect); pir; pir++)
  {
    double value = alpha * acc_x.read(DomainPoint::from_point<1>(pir.p)) + 
      acc_y.read(DomainPoint::from_point<1>(pir.p));
    acc_z.write(DomainPoint::from_point<1>(pir.p), value);
  }
}

void check_task(const Task *task,
    const std::vector<PhysicalRegion> &regions,
    Context ctx, Runtime *runtime)
{
  assert(regions.size() == 2);
  assert(task->regions.size() == 2);
  assert(task->arglen == sizeof(double));
  const double alpha = *((const double*)task->args);
  RegionAccessor<AccessorType::Generic, double> acc_x = 
    regions[0].get_field_accessor(FID_X).typeify<double>();
  RegionAccessor<AccessorType::Generic, double> acc_y = 
    regions[0].get_field_accessor(FID_Y).typeify<double>();
  RegionAccessor<AccessorType::Generic, double> acc_z = 
    regions[1].get_field_accessor(FID_Z).typeify<double>();
  printf("Checking results...");
  Domain dom = runtime->get_index_space_domain(ctx, 
      task->regions[0].region.get_index_space());
  Rect<1> rect = dom.get_rect<1>();
  bool all_passed = true;
  for (GenericPointInRectIterator<1> pir(rect); pir; pir++)
  {
    double expected = alpha * acc_x.read(DomainPoint::from_point<1>(pir.p)) + 
      acc_y.read(DomainPoint::from_point<1>(pir.p));
    double received = acc_z.read(DomainPoint::from_point<1>(pir.p));

    if (expected != received)
      all_passed = false;
  }
  if (all_passed)
    printf("SUCCESS!\n");
  else
    printf("FAILURE!\n");
}

int main(int argc, char **argv)
{
  Runtime::set_top_level_task_id(TOP_LEVEL_TASK_ID);
  Runtime::register_legion_task<top_level_task>(TOP_LEVEL_TASK_ID,
      Processor::LOC_PROC, true/*single*/, false/*index*/,
      AUTO_GENERATE_ID, TaskConfigOptions(), "top_level");

  Runtime::register_legion_task<init_field_task>(INIT_FIELD_TASK_ID,
      Processor::LOC_PROC, true/*single*/, true/*index*/,
      AUTO_GENERATE_ID, TaskConfigOptions(true), "init_field");
  Runtime::register_legion_task<daxpy_task>(DAXPY_TASK_ID,
      Processor::LOC_PROC, true/*single*/, true/*index*/,
      AUTO_GENERATE_ID, TaskConfigOptions(true), "daxpy");
  Runtime::register_legion_task<check_task>(CHECK_TASK_ID,
      Processor::LOC_PROC, true/*single*/, true/*index*/,
      AUTO_GENERATE_ID, TaskConfigOptions(true), "check");

  // Here is where we register the callback function for 
  // creating custom mappers.
  Runtime::set_registration_callback(mapper_registration);

  return Runtime::start(argc, argv);
}
