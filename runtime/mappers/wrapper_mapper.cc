#include <cstdlib>
#include <cassert>
#include <algorithm>

#define STATIC_MAX_PERMITTED_STEALS   4
#define STATIC_MAX_STEAL_COUNT        2
#define STATIC_SPLIT_FACTOR           2
#define STATIC_BREADTH_FIRST          false
#define STATIC_WAR_ENABLED            false 
#define STATIC_STEALING_ENABLED       false
#define STATIC_MAX_SCHEDULE_COUNT     8
#define STATIC_NUM_PROFILE_SAMPLES    1
#define STATIC_MAX_FAILED_MAPPINGS    8


#include "wrapper_mapper.h"


namespace LegionRuntime {
	namespace HighLevel {
		std::vector<int> WrapperMapper::tasks_list;
		std::vector<int> WrapperMapper::functions_list;
		std::set<Memory> WrapperMapper::all_mems;
		std::set<Processor> WrapperMapper::all_procs;
		std::vector<Memory> WrapperMapper::mems_list;
		std::vector<Processor> WrapperMapper::procs_list;
		std::map<int, int> WrapperMapper::methods_map;
		std::map<Processor, int> WrapperMapper::procs_map;
		std::map<std::string, int> WrapperMapper::tasks_map;
		std::map<Memory, int> WrapperMapper::mems_map;
		std::map<int, std::string> WrapperMapper::task_names_map;
		bool WrapperMapper::inputtaken=0;

		WrapperMapper::WrapperMapper(Mapper* dmapper,Machine machine, HighLevelRuntime *rt, Processor local):Mapper(rt),dmapper(dmapper), local_proc(local), 
		local_kind(local.kind()), machine(machine),
		max_steals_per_theft(STATIC_MAX_PERMITTED_STEALS),
		max_steal_count(STATIC_MAX_STEAL_COUNT),
		splitting_factor(STATIC_SPLIT_FACTOR),
		breadth_first_traversal(STATIC_BREADTH_FIRST),
		war_enabled(STATIC_WAR_ENABLED),
		stealing_enabled(STATIC_STEALING_ENABLED),
		max_schedule_count(STATIC_MAX_SCHEDULE_COUNT),
		max_failed_mappings(STATIC_MAX_FAILED_MAPPINGS),
		machine_interface(MappingUtilities::MachineQueryInterface(machine)){
			machine.get_all_processors(WrapperMapper::all_procs);
			machine.get_all_memories(WrapperMapper::all_mems);
			if (!WrapperMapper::inputtaken){
				WrapperMapper::get_input();
				WrapperMapper::inputtaken=1;
			}
		}
		WrapperMapper::~WrapperMapper(){
		}
		bool is_number(const std::string& s)
		{
			std::string::const_iterator it = s.begin();
			while (it != s.end() && std::isdigit(*it)) ++it;
			return !s.empty() && it == s.end();
		}

		bool is_valid_name(const std::string& s){
			std::map<int,std::string>::iterator it = WrapperMapper::task_names_map.begin();
			while (it!=WrapperMapper::task_names_map.end()){
				if (s==it->second) break;
				++it;
			}
			if (it!=WrapperMapper::task_names_map.end()) return 1;
			else return 0;
		}

		void WrapperMapper::get_input(){
			std::string strValue;
			std::map<int, std::string> function_map;
			int Value, pValue;

			function_map[1] = "select_task_options"; function_map[2] = "select_tasks_to_schedule";
			function_map[3] = "target_task_steal"; function_map[4] = "permit_task_steal";
			function_map[5] = "slice_domain"; function_map[6] = "pre_map_task";
			function_map[7] = "select_task_variant"; function_map[8] = "map_task";
			function_map[9] = "post_map_task"; function_map[10] = "map_copy";
			function_map[11] = "map_inline"; function_map[12] = "map_must_epoch";
			function_map[13] = "notify_mapping_result"; function_map[14] = "notify_mapping_failed";
			function_map[15] = "rank_copy_targets"; function_map[16] = "rank_copy_sources";
			function_map[17] = "Other";

			std::cout<< "Type 'help' to see the list of commands. Type 'exit' to exit.\n";
			std::cout<<">    ";
			while (1)
			{
				getline(std::cin, strValue); 
				std::string nameValue;
				std::string intValue;
				if (strValue.compare(0,12,"print task +")==0){
					nameValue=strValue.substr(12);
					if(is_valid_name(nameValue)){
						std::map<std::string, int>::iterator it = WrapperMapper::tasks_map.find(nameValue);
						if (it==WrapperMapper::tasks_map.end())
						{
							pValue=1;
							WrapperMapper::tasks_map.insert(std::pair<std::string, int>(nameValue,pValue));
							std::cout<<"The tasks added are: ";
							for (std::map<std::string, int>::const_iterator i = WrapperMapper::tasks_map.begin(); i != WrapperMapper::tasks_map.end(); ++i) std::cout<< i->first << "  ";
							std::cout<<"\n>    ";
						}
						else{
							WrapperMapper::tasks_map.erase(it);
							pValue=1;
							WrapperMapper::tasks_map.insert(std::pair<std::string, int>(nameValue,pValue));
							std::cout<<"The tasks added are: ";
							for (std::map<std::string, int>::const_iterator i = WrapperMapper::tasks_map.begin(); i != WrapperMapper::tasks_map.end(); ++i) std::cout<< i->first << "  ";
							std::cout<<"\n>    ";
						}
					}
					else std::cout<<"No task of that name\n>    ";
				}

				else if (strValue.compare(0,11,"stop task +")==0){
					nameValue=strValue.substr(11);
					if(is_valid_name(nameValue)){
						std::map<std::string, int>::iterator it = WrapperMapper::tasks_map.find(nameValue);
						if (it==WrapperMapper::tasks_map.end())
						{
							pValue=0;
							WrapperMapper::tasks_map.insert(std::pair<std::string, int>(nameValue,pValue));
							std::cout<<"The tasks added are: ";
							for (std::map<std::string, int>::const_iterator i = WrapperMapper::tasks_map.begin(); i != WrapperMapper::tasks_map.end(); ++i) std::cout<< i->first << "  ";
							std::cout<<"\n>    ";
						}
						else{
							WrapperMapper::tasks_map.erase(it);
							pValue=0;
							WrapperMapper::tasks_map.insert(std::pair<std::string, int>(nameValue,pValue));
							std::cout<<"The tasks added are: ";
							for (std::map<std::string, int>::const_iterator i = WrapperMapper::tasks_map.begin(); i != WrapperMapper::tasks_map.end(); ++i) std::cout<< i->first << "  ";
							std::cout<<"\n>    ";
						}
					}
					else std::cout<<"No task of that name\n>    ";
				}

				else if (strValue.compare(0,14,"print method +")==0){
					intValue=strValue.substr(14);
					if(InputNumberCheck(intValue)){
						Value = std::atoi(intValue.c_str());
						if (Value>0 && Value<18){ 
							std::map<int, int>::iterator it = WrapperMapper::methods_map.find(Value);
							if (it==WrapperMapper::methods_map.end()){
								pValue=1;
								WrapperMapper::methods_map.insert(std::pair<int, int>(Value,pValue));
								std::cout<<"The methods added are: ";
								for (std::map<int, int>::const_iterator i = WrapperMapper::methods_map.begin(); i != WrapperMapper::methods_map.end(); ++i) std::cout<< function_map[i->first] << "  ";
								std::cout<<"\n>    ";
							}
							else{
								WrapperMapper::methods_map.erase(it);
								pValue=1;
								WrapperMapper::methods_map.insert(std::pair<int, int>(Value,pValue));
								std::cout<<"The methods added are: ";
								for (std::map<int, int>::const_iterator i = WrapperMapper::methods_map.begin(); i != WrapperMapper::methods_map.end(); ++i) std::cout<< function_map[i->first] << "  ";
								std::cout<<"\n>    ";
							}
						}
						else std::cout<<"Method number should be between 1 and 17\n>    ";
					}
					else std::cout<<"Method ID not a number\n>    ";
				}

				else if (strValue.compare(0,13,"stop method +")==0){
					intValue=strValue.substr(13);
					if(InputNumberCheck(intValue)){
						Value = std::atoi(intValue.c_str());
						if (Value>0 && Value<18){ 
							std::map<int, int>::iterator it = WrapperMapper::methods_map.find(Value);
							if (it==WrapperMapper::methods_map.end()){
								pValue=0;
								WrapperMapper::methods_map.insert(std::pair<int, int>(Value,pValue));
								std::cout<<"The methods added are: ";
								for (std::map<int, int>::const_iterator i = WrapperMapper::methods_map.begin(); i != WrapperMapper::methods_map.end(); ++i) std::cout<< function_map[i->first] << "  ";
								std::cout<<"\n>    ";
							}
							else{
								WrapperMapper::methods_map.erase(it);
								pValue=0;
								WrapperMapper::methods_map.insert(std::pair<int, int>(Value,pValue));
								std::cout<<"The methods added are: ";
								for (std::map<int, int>::const_iterator i = WrapperMapper::methods_map.begin(); i != WrapperMapper::methods_map.end(); ++i) std::cout<< function_map[i->first] << "  ";
								std::cout<<"\n>    ";
							}
						}
						else std::cout<<"Method number should be between 1 and 17\n>    ";
					}
					else std::cout<<"Method ID not a number\n>    ";
				}

				else if (strValue.compare(0,17,"print processor +")==0){
					intValue=strValue.substr(17);
					std::set<Processor>::iterator it;
					if (is_number(intValue)){
						int i=std::atoi(intValue.c_str())-1;
						if ((unsigned)i<WrapperMapper::all_procs.size()){
							it = WrapperMapper::all_procs.begin();
							std::advance(it, i);
							std::map<Processor, int>::iterator ite= WrapperMapper::procs_map.find(*it);
							if (ite!=WrapperMapper::procs_map.end() ) WrapperMapper::procs_map.erase(ite);				
							pValue=1;
							WrapperMapper::procs_map.insert(std::pair<Processor,int>(*it,pValue));
							
							std::cout<<"The processors added are: ";
							for (std::map<Processor,int>::const_iterator it = WrapperMapper::procs_map.begin(); it != WrapperMapper::procs_map.end(); ++it) std::cout<< it->first.id << "   ";
							std::cout<<"\n>    ";
						}
						else std::cout<<"Invalid number entered\n>    ";
					}
					else std::cout<<"Invalid input\n>    ";			
				}

				else if (strValue.compare(0,16,"stop processor +")==0){
					intValue=strValue.substr(16);
					std::set<Processor>::iterator it;
					if (is_number(intValue)){
						int i=std::atoi(intValue.c_str())-1;
						if ((unsigned)i<WrapperMapper::all_procs.size()){
							it = WrapperMapper::all_procs.begin();
							std::advance(it, i);
							std::map<Processor, int>::iterator ite= WrapperMapper::procs_map.find(*it);
							if (ite!=WrapperMapper::procs_map.end()) WrapperMapper::procs_map.erase(ite);				
							pValue=0;
							WrapperMapper::procs_map.insert(std::pair<Processor,int>(*it,pValue));
							
							std::cout<<"The processors added are: ";
							for (std::map<Processor,int>::const_iterator it = WrapperMapper::procs_map.begin(); it != WrapperMapper::procs_map.end(); ++it) std::cout<< it->first.id << "   ";
							std::cout<<"\n>    ";
						}
						else std::cout<<"Invalid number entered\n>    ";
					}
					else std::cout<<"Invalid input\n>    ";			
				}
				
				else if (strValue.compare(0,14,"print memory +")==0){
					intValue=strValue.substr(14);
					std::set<Memory>::iterator it;
					if (is_number(intValue)){
						int i=std::atoi(intValue.c_str())-1;
						if((unsigned)i<WrapperMapper::all_mems.size()){
							it = WrapperMapper::all_mems.begin();
							std::advance(it,i);
							std::map<Memory, int>::iterator itm = WrapperMapper::mems_map.find(*it);
							if (itm!=WrapperMapper::mems_map.end()) WrapperMapper::mems_map.erase(itm);
							pValue=1;									
							WrapperMapper::mems_map.insert(std::pair<Memory,int>(*it,pValue));
							
							std::cout<<"The memories added are: ";
							for (std::map<Memory,int>::const_iterator it = WrapperMapper::mems_map.begin(); it != WrapperMapper::mems_map.end(); ++it) std::cout<<it->first.id<<"		";
							std::cout<<"\n>    ";;
						}
						else std::cout<<"Invalid number entered\n>    ";
					}
					else std::cout<<"Invalid input\n>    ";
				}
				
				else if (strValue.compare(0,13,"stop memory +")==0){
					intValue=strValue.substr(13);
					std::set<Memory>::iterator it;
					if (is_number(intValue)){
						int i=std::atoi(intValue.c_str())-1;
						if((unsigned)i<WrapperMapper::all_mems.size()){
							it = WrapperMapper::all_mems.begin();
							std::advance(it,i);
							std::map<Memory, int>::iterator itm = WrapperMapper::mems_map.find(*it);
							if (itm!=WrapperMapper::mems_map.end()) WrapperMapper::mems_map.erase(itm);
							pValue=0;									
							WrapperMapper::mems_map.insert(std::pair<Memory,int>(*it,pValue));
							
							std::cout<<"The memories added are: ";
							for (std::map<Memory,int>::const_iterator it = WrapperMapper::mems_map.begin(); it != WrapperMapper::mems_map.end(); ++it) std::cout<<it->first.id<<"		";
							std::cout<<"\n>    ";;
						}
						else std::cout<<"Invalid number entered\n>    ";
					}
					else std::cout<<"Invalid input\n>    ";
				}
				else if (strValue.compare(0,6,"task -")==0){
					nameValue=strValue.substr(6);
					if(is_valid_name(nameValue)){
						std::map<std::string, int>::iterator it = WrapperMapper::tasks_map.find(nameValue);
						if (it!=WrapperMapper::tasks_map.end())
						{WrapperMapper::tasks_map.erase(it);
							std::cout<<"The tasks added are: ";
							for (std::map<std::string, int>::const_iterator i = WrapperMapper::tasks_map.begin(); i != WrapperMapper::tasks_map.end(); ++i) std::cout<< i->first << "  ";
							std::cout<<"\n>    ";
						}
						else std::cout<<"Task "<<Value<<" not present\n>    ";
					}
					else std::cout<<"Task ID not a number\n>    ";
				}
				

				else if (strValue.compare(0,8,"method -")==0){
					intValue=strValue.substr(8);
					if(InputNumberCheck(intValue)){
						Value = std::atoi(intValue.c_str());
						if (Value>0 && Value<18){
							std::map<int, int>::iterator it = WrapperMapper::methods_map.find(Value);
							if (it!=WrapperMapper::methods_map.end()){
								WrapperMapper::methods_map.erase(it);				
								std::cout<<"The methods added are: ";
								for (std::map<int, int>::const_iterator i = WrapperMapper::methods_map.begin(); i != WrapperMapper::methods_map.end(); ++i) std::cout<< function_map[i->first] << "  ";
								std::cout<<"\n>    ";
							}
							else std::cout<<"Method not present.\n>    "; 
						}
						else std::cout<<"Method number should be between 1 and 17\n>    ";
					}
					else std::cout<<"Method ID not a number\n>    ";
				}
				
				else if (strValue.compare(0,11,"processor -")==0){
					intValue=strValue.substr(11);
					std::set<Processor>::iterator it;
					std::map<Processor, int>::iterator ite;
					if (is_number(intValue)){
						int i=std::atoi(intValue.c_str())-1;
						if ((unsigned)i<WrapperMapper::all_procs.size()){
							it = WrapperMapper::all_procs.begin();
							std::advance(it, i);
							std::map<Processor, int>::iterator ite= WrapperMapper::procs_map.find(*it);
							if (ite!=WrapperMapper::procs_map.end() ){
								WrapperMapper::procs_map.erase(ite);				
								std::cout<<"The processors added are: ";
								for (std::map<Processor,int>::const_iterator it = WrapperMapper::procs_map.begin(); it != WrapperMapper::procs_map.end(); ++it) std::cout<< it->first.id << "   ";
								std::cout<<"\n>    ";
							}
						}
						else std::cout<<"Invalid number entered\n>    ";
					}
					else std::cout<<"Invalid input\n>    ";

				}
				
				else if (strValue.compare(0,8,"memory -")==0){
					intValue=strValue.substr(8);
					std::set<Memory>::iterator it;
					std::map<Memory, int>::iterator ite;
					if(is_number(intValue)){
						int i=std::atoi(intValue.c_str())-1;
						if((unsigned)i<WrapperMapper::all_mems.size()){
							it = WrapperMapper::all_mems.begin();
							std::advance(it, i);
							std::map<Memory, int>::iterator ite=WrapperMapper::mems_map.find(*it);
							if(ite!=WrapperMapper::mems_map.end()){
								WrapperMapper::mems_map.erase(ite);
								std::cout<<"The memories added are:	";
								for (std::map<Memory,int>::const_iterator it = WrapperMapper::mems_map.begin(); it!=WrapperMapper::mems_map.end(); ++it) std::cout<<it->first.id<<"		";
								std::cout<<"\n>    ";
							}
						}
						else std::cout<<"Invalid number entered\n>    ";
					}
					else std::cout<<"Invalid input\n>    ";
					
					
				}
				
				else if (strValue.compare("help")==0){
					std::cout<<"Following are the commands that can be executed:\n";
					std::cout<<"task +<task_id> --> To add a task to be monitored \n";
					std::cout<<"task -<task_id> --> To remove a task from the lists of tasks which are being monitored \n";
					std::cout<<"methods --> To see the list of methods with their corresponding ids\n";
					std::cout<<"method +<method_id> --> To add a method to be monitored\n";
					std::cout<<"method -<method_id> --> To remove a method from the lists of methods which are being monitored \n";
					std::cout<<"processors --> To see the list of processor with their corresponding ids\n";
					std::cout<<"processor +<processor_id> --> To add a processor to be monitored\n";
					std::cout<<"processor -<processor_id> --> To remove a processor from the lists of processors which are being monitored \n";
					std::cout<<">    ";
				}

				else if (strValue.compare("tasks")==0){
					std::cout<<"Task ID"<<"	"<<"Task Name"<<"\n";
					for (std::map<int, std::string>::iterator it = WrapperMapper::task_names_map.begin(); it!=WrapperMapper::task_names_map.end(); it++) std::cout<<it->first<<"	"<<it->second<<"\n";
					std::cout<<">    ";
				}				

				else if (strValue.compare("methods")==0){
					for(std::map<int, std::string >::const_iterator it = function_map.begin(); it != function_map.end(); ++it)
					{
						std::cout << it->first << ". " << it->second << " " << "\n";
					}
					std::cout<<">    ";
				}
				
				else if (strValue.compare("processors")==0){
					int i=0;
					std::set<Processor>::iterator it;
					for ( it = WrapperMapper::all_procs.begin();
					it != WrapperMapper::all_procs.end(); it++)
					{
						i++;
						Processor::Kind k = it->kind();
						if (k == Processor::UTIL_PROC) std::cout<<i<<". Utility Processor ID:"<<it->id<<"\n";
						else std::cout<<i<<". Processor ID: "<<it->id<<"  Kind:"<<k<<"\n";
					}
					std::cout<<">    ";
				}
				
				else if (strValue.compare("memories")==0){
					int i=0;
					std::set<Memory>::iterator it;
					for ( it = WrapperMapper::all_mems.begin();
					it != WrapperMapper::all_mems.end(); it++)
					{
						i++;
						std::cout<<i<<". Memory ID: "<<it->id<<"  Capacity: "<<it->capacity()<<"  Kind: "<<it->kind()<<"\n";
					}
					std::cout<<">    ";
				}
				
				else if (strValue.compare("exit")==0) break;
				
				else std::cout<<"Invalid Command\n>    ";
				
			}
		}

		void WrapperMapper::get_select_task_options_input(Task *task){
			std::string strValue;
			std::cout<<"\nType change to change the list of tasks and methods being monitored. Type help for the list of commands. Type exit to exit\n";
			std::cout<<"\nTo change a task option, enter the the number corresponding to the option:\n";
			std::cout<<"1. target processor\n2. inline task\n3. spawn task\n4. map locally\n>    ";
			while(1){
				getline(std::cin, strValue);
				if (strValue.compare("1")==0){
					int i=0;
					std::set<Processor>::iterator it;
					for ( it = WrapperMapper::all_procs.begin();
					it != WrapperMapper::all_procs.end(); it++)
					{
						i++;
						Processor::Kind k = it->kind();
						if (k == Processor::UTIL_PROC)
						std::cout<<i<<". Utility Processor ID:"<<it->id<<"\n";
						else
						std::cout<<i<<". Processor ID: "<<it->id<<"Kind:"<<k<<"\n";
					}
					std::cout<<"Enter the number corresponding to the processor to be selected\n>    ";
					while(1){
						getline(std::cin, strValue);
						if (is_number(strValue)){
							i=std::atoi(strValue.c_str())-1;
							if ((unsigned)i<WrapperMapper::all_procs.size()){
								it = WrapperMapper::all_procs.begin();
								std::advance(it, i);
								task->target_proc= *it;
								std::cout<<"\ntarget processor="<<task->target_proc.id<<"\n";
								break;
							}
							else std::cout<<"Invalid number entered\n>    ";
						}
						else std::cout<<"Invalid input\n>    ";
					}
					std::cout<<"\n>    ";
				}
				else if (strValue.compare("2")==0){
					std::cout<<"Enter 0 or 1\n>    ";
					std::string strValue1;
					while(1){
						getline(std::cin, strValue1);
						if (strValue1=="0" || strValue1=="1"){
							task->inline_task=atoi(strValue1.c_str());	
							std::cout<<"\ninline task="<<task->inline_task<<"\n";
							break;
						}
						else std::cout<<"Invalid input\n>    ";
					}
					std::cout<<"\n>    ";
				}
				else if (strValue.compare("3")==0){
					std::cout<<"Enter 0 or 1\n>    ";
					std::string strValue1;
					while(1){
						getline(std::cin, strValue1);
						if (strValue1=="0" || strValue1=="1"){
							task->spawn_task=atoi(strValue1.c_str());
							std::cout<<"\nspawn task="<<task->spawn_task<<"\n";
							break;
						}
						else std::cout<<"Invalid input\n>    ";
					}
					std::cout<<"\n>    ";
				}
				else if (strValue.compare("4")==0){
					std::cout<<"Enter 0 or 1\n>    ";
					std::string strValue1;
					while(1){
						getline(std::cin, strValue1);
						if (strValue1=="0" || strValue1=="1"){
							task->map_locally=atoi(strValue1.c_str());
							std::cout<<"\nmap locally="<<task->map_locally<<"\n";
							break;
						}
						else std::cout<<"Invalid input\n>    ";
					}
					std::cout<<"\n>    ";
				}
				else if (strValue.compare("change")==0){
					WrapperMapper::get_input();
					std::cout<<"\n>    ";
				}
				else if (strValue.compare("exit")==0) break;
				else std::cout<<"Invalid input\n>    ";
			}
		}

		void WrapperMapper::get_map_task_input(Task *task){
			std::string strValue;
			std::cout<< "Type change to change the list of tasks and methods being monitored. Type 'exit' to exit.\n>    ";			
			while (1)
			{
				getline(std::cin, strValue); 
				if (strValue.compare("change")==0){
					WrapperMapper::get_input();
					std::cout<<"\n>    ";
				}
				else if (strValue.compare("exit")==0) break;
				else std::cout<<"Invalid input\n>    ";
			}
		}

		void WrapperMapper::select_task_options(Task *task){
			dmapper->select_task_options(task);
			std::map<std::string, int>::iterator itt = WrapperMapper::tasks_map.find(WrapperMapper::task_names_map[task->task_id]); 
			std::map<int, int>::iterator itm = WrapperMapper::methods_map.find(1);
			std::map<Processor, int>::iterator itp = WrapperMapper::procs_map.find(task->target_proc);
			if (((itt!=WrapperMapper::tasks_map.end()) && (itm!=WrapperMapper::methods_map.end())) || (itp!=WrapperMapper::procs_map.end())) {
				std::cout<<"\n--------------TASK: "<<WrapperMapper::task_names_map[task->task_id]<<" FUNCTION: select_task_options--------------\n";
				std::cout<<"\nThe selected task options for task "<<WrapperMapper::task_names_map[task->task_id]<<" are as follows:\n";
				std::cout<<"target processor="<<task->target_proc.id<<"\ninline task="<<task->inline_task;
				std::cout<<"\nspawn task="<<task->spawn_task<<"\nmap locally="<<task->map_locally<<"\n\n";
				if (((itt!=tasks_map.end() && itt->second==0) && (itm!=methods_map.end() && itm->second==0))||(itp!=procs_map.end() && itp->second==0)) {
					std::cout<<"To change the task options, type 'change' and to exit, type 'exit'\n";
					WrapperMapper::get_select_task_options_input(task);
				}
			}
		}

		void WrapperMapper::select_tasks_to_schedule(const std::list<Task*> &ready_tasks){
			bool print;
			if (std::find(WrapperMapper::functions_list.begin(),WrapperMapper::functions_list.end(),2) != WrapperMapper::functions_list.end() ) print=true;
			else print=false;
			if (print){
				std::cout<<"\nSelecting tasks to schedule:\n";
				WrapperMapper::get_input();
			}
			dmapper->select_tasks_to_schedule(ready_tasks);
			if (print){
				std::cout<<"\ntest postprint select_tasks_to_schedule:\n";
			}
		}

		void WrapperMapper::target_task_steal(const std::set<Processor> &blacklist,std::set<Processor> &targets){
			bool print;
			if (std::find(WrapperMapper::functions_list.begin(),WrapperMapper::functions_list.end(),3) != WrapperMapper::functions_list.end() ) print=true;
			else print=false;
			if (print){
				std::cout<<"\ntest preprint target_task_steal\n";
				WrapperMapper::get_input();
			}
			dmapper->target_task_steal(blacklist,targets);
			if (print){
				std::cout<<"\ntest postprint target_task_steal\n";
			}
		}

		void WrapperMapper::permit_task_steal(Processor thief, const std::vector<const Task*> &tasks, std::set<const Task*> &to_steal){
			bool print;
			if (std::find(WrapperMapper::functions_list.begin(),WrapperMapper::functions_list.end(),4) != WrapperMapper::functions_list.end() ) print=true;
			else print=false;
			if(print){
				std::cout<<"\ntest preprint permit_task_steal\n";
				WrapperMapper::get_input();
			}
			dmapper->permit_task_steal(thief, tasks, to_steal);
			if(print){
				std::cout<<"\ntest postprint permit_task_steal\n";
			}
		}

		//--------------------------------------------------------------------------
		void WrapperMapper::slice_domain(const Task *task, const Domain &domain,
		std::vector<DomainSplit> &slices)
		//--------------------------------------------------------------------------
		{
			bool print;
			if ( std::find(WrapperMapper::tasks_list.begin(),WrapperMapper::tasks_list.end(), task->task_id) != WrapperMapper::tasks_list.end() && std::find(WrapperMapper::functions_list.begin(),WrapperMapper::functions_list.end(),5) != WrapperMapper::functions_list.end()) print = true;
			else print=false;
			if (print){
				std::cout<<"\ntest preprint slice_domain\n";
				WrapperMapper::get_input();
			}
			dmapper->slice_domain(task, domain, slices);
			if(print){
				std::cout<<"\ntest postprint slice_domain\n";
			}
		}

		bool WrapperMapper::pre_map_task(Task *task){
			bool print;
			if ( std::find(WrapperMapper::tasks_list.begin(),WrapperMapper::tasks_list.end(), task->task_id) != WrapperMapper::tasks_list.end() && std::find(WrapperMapper::functions_list.begin(),WrapperMapper::functions_list.end(),6) != WrapperMapper::functions_list.end() ) print = true;
			else print=false;
			if (print){
				std::cout<<"\ntest preprint pre_map_task\n";
				WrapperMapper::get_input();
			}
			bool result=dmapper->pre_map_task(task);
			if(print){
				std::cout<<"\ntest postprint pre_map_task"<<result<<"   "<<task->task_id<<"\n";
			}
			return result;
		}


		void WrapperMapper::select_task_variant(Task *task){bool print;
			if ( std::find(WrapperMapper::tasks_list.begin(),WrapperMapper::tasks_list.end(), task->task_id) != WrapperMapper::tasks_list.end() && std::find(WrapperMapper::functions_list.begin(),WrapperMapper::functions_list.end(),7) != WrapperMapper::functions_list.end() ) print = true;
			else print=false;
			if (print){
				std::cout<<"\ntest preprint select_task_variant\n";
				WrapperMapper::get_input();
			}
			dmapper->select_task_variant(task);
			if(print){
				std::cout<<"\ntest postprint select_task_variant  "<<task->task_id<<"\n";
			}
		}

		bool WrapperMapper::map_task(Task *task){
			bool result=dmapper->map_task(task);
			std::map<std::string, int>::iterator itt = WrapperMapper::tasks_map.find(WrapperMapper::task_names_map[task->task_id]); 
			std::map<int, int>::iterator itm = WrapperMapper::methods_map.find(8);
			std::vector<Memory> mems_vec;
			int mem_print=0;
			int mem_stop=0;
			std::map<Memory, int>::iterator itme;
			for (unsigned idx = 0; idx < task->regions.size(); idx++)
			{
				mems_vec=task->regions[idx].target_ranking;
				for (std::vector<Memory>::const_iterator it=mems_vec.begin(); it!=mems_vec.end(); ++it){
					itme= WrapperMapper::mems_map.find(*it);
					if (itme!=WrapperMapper::mems_map.end()) {
						if (!mem_print) mem_print=1;
						if (!mem_stop && itme->second==0) mem_stop=1;
					}
				}
			}
			if (((itt!=WrapperMapper::tasks_map.end()) && (itm!=WrapperMapper::methods_map.end())) || mem_print) {	
				std::cout<<"\n--------------TASK: "<<WrapperMapper::task_names_map[task->task_id]<<"		PROCESSOR: "<<task->target_proc.id<<"	FUNCTION: map_task--------------\n";
				for (unsigned idx = 0; idx < task->regions.size(); idx++)
				{
					mems_vec=task->regions[idx].target_ranking;
					std::cout<<"Region  "<<idx+1<<"\n";
					int t=0;
					for (std::vector<Memory>::const_iterator it=mems_vec.begin(); it!=mems_vec.end(); ++it){
						t++;
						std::cout<<t<<".Memory ID: "<<it->id<<"  Capacity: "<<it->capacity()<<"  Kind: "<<it->kind()<<"\n";
					}
				}
			}
			if(mem_stop) WrapperMapper::get_map_task_input(task);
			return result;
		}

		void WrapperMapper::post_map_task(Task *task){
			bool print;
			if ( std::find(WrapperMapper::tasks_list.begin(),WrapperMapper::tasks_list.end(), task->task_id) != WrapperMapper::tasks_list.end() && std::find(WrapperMapper::functions_list.begin(),WrapperMapper::functions_list.end(),9) != WrapperMapper::functions_list.end()) print = true;
			else print=false;
			if (print){
				std::cout<<"\ntest preprint post_map_task\n";
				WrapperMapper::get_input();
			}
			dmapper->post_map_task(task);
			if(print){
				std::cout<<"\ntest postprint post_map_task\n";
			}
		}

		bool WrapperMapper::map_copy(Copy *copy){
			bool print;
			if ( std::find(WrapperMapper::functions_list.begin(),WrapperMapper::functions_list.end(),10) != WrapperMapper::functions_list.end()) print = true;
			else print=false;
			if (print){
				std::cout<<"\ntest postprint map_copy\n";
			}
			bool result=dmapper->map_copy(copy);
			if(print){
				std::cout<<"\ntest postprint map_copy\n";
			}
			return result;
		}

		bool WrapperMapper::map_inline(Inline *inline_operation){
			return dmapper->map_inline(inline_operation);
		}

		bool WrapperMapper::map_must_epoch(const std::vector<Task*> &tasks,
		const std::vector<MappingConstraint> &constraints,
		MappingTagID tag)
		//--------------------------------------------------------------------------
		{
			return dmapper->map_must_epoch(tasks, constraints, tag);
		}


		void WrapperMapper::notify_mapping_result(const Mappable *mappable){
			dmapper->notify_mapping_result(mappable);
		}

		void WrapperMapper::notify_mapping_failed(const Mappable *mappable){
			dmapper->notify_mapping_failed(mappable);
		}

		bool WrapperMapper::rank_copy_targets(const Mappable *mappable, LogicalRegion rebuild_region, const std::set<Memory> &current_instances, bool complete, size_t max_blocking_factor, std::set<Memory> &to_reuse, std::vector<Memory> &to_create, bool &create_one, size_t &blocking_factor){
			return dmapper->rank_copy_targets(mappable, rebuild_region, current_instances, complete, max_blocking_factor, to_reuse, to_create, create_one, blocking_factor);
		}

		void WrapperMapper::rank_copy_sources(const Mappable *mappable, const std::set<Memory> &current_instances, Memory dst_mem, std::vector<Memory> &chosen_order){
			dmapper-> rank_copy_sources(mappable,current_instances, dst_mem, chosen_order);
		}

		void WrapperMapper::notify_profiling_info(const Task *task){
			dmapper->notify_profiling_info(task);
		}

		bool WrapperMapper::speculate_on_predicate(const Mappable *mappable,bool &spec_value){
			return dmapper->speculate_on_predicate(mappable, spec_value);
		}

		void WrapperMapper::configure_context(Task *task){
			dmapper->configure_context(task);
		}

		int WrapperMapper::get_tunable_value(const Task *task, TunableID tid, MappingTagID tag){
			return dmapper->get_tunable_value(task, tid, tag);
		}

		void WrapperMapper::handle_message(Processor source, const void *message, size_t length){
			dmapper->handle_message(source, message, length);
		}

		void WrapperMapper::handle_mapper_task_result(MapperEvent event, const void *result, size_t result_size){
			dmapper->handle_mapper_task_result(event, result, result_size);
		}

		bool WrapperMapper::InputMatches(std::string strUserInput, std::string strTemplate)
		{
			if (strTemplate.length() != strUserInput.length())
			return false;

			// Step through the user input to see if it matches
			for (unsigned int nIndex=0; nIndex < strTemplate.length(); nIndex++)
			{
				switch (strTemplate[nIndex])
				{
				case '#': // match a digit
					if (!std::isdigit(strUserInput[nIndex]))
					return false;
					break;
				case '_': // match a whitespace
					if (!std::isspace(strUserInput[nIndex]))
					return false;
					break;
				case '@': // match a letter
					if (!std::isalpha(strUserInput[nIndex]))
					return false;
					break;
				case '?': // match anything
					break;
				default: // match the exact character
					if (strUserInput[nIndex] != strTemplate[nIndex])
					return false;
				}
			}
			return true;
		}

		bool WrapperMapper::InputNumberCheck(std::string strUserInput)
		{
			for (unsigned int nIndex=0; nIndex < strUserInput.length(); nIndex++)
			{
				if (!std::isdigit(strUserInput[nIndex])) return false;
			}
			return true;
		}


	};
};

