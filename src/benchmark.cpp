#include "benchmark.h"


void LoadBalancingProbeTask::run() {
	distribute_probe.start();
	lb_task.run();
	distribute_probe.stop();

	monitor.commit(distribute_probe);
}

fpmas::graph::PartitionMap LoadBalancingProbe::balance(
		fpmas::graph::NodeMap<fpmas::model::AgentPtr> node_map,
		fpmas::api::graph::PartitionMode mode) {
	balance_probe.start();
	
	auto result = lb.balance(node_map, mode);

	balance_probe.stop();
	monitor.commit(balance_probe);

	return result;
}

TestCase::TestCase(
		std::string lb_algorithm_name, BenchmarkConfig config,
		fpmas::api::scheduler::Scheduler& scheduler,
		fpmas::api::runtime::Runtime& runtime,
		fpmas::api::model::LoadBalancing& lb_algorithm,
		fpmas::scheduler::TimeStep lb_period
		) :
	lb_algorithm_name(lb_algorithm_name + "-" + std::to_string(lb_period)),
	model(scheduler, runtime, lb_algorithm),
	lb_probe(model.graph(), lb_algorithm), csv_output(*this),
	cells_output(model, this->lb_algorithm_name, config.grid_width, config.grid_height),
	agents_output(model, this->lb_algorithm_name, config.grid_width, config.grid_height),
	config(config) {
		auto& cell_group = model.buildGroup(CELL_GROUP, cell_behavior);
		auto& create_relations_neighbors_group = model.buildGroup(
				RELATIONS_FROM_NEIGHBORS_GROUP, create_relations_from_neighborhood
				);
		auto& create_relations_contacts_group = model.buildGroup(
				RELATIONS_FROM_CONTACTS_GROUP, create_relations_from_contacts
				);
		auto& handle_new_contacts_group = model.buildGroup(
				HANDLE_NEW_CONTACTS_GROUP, handle_new_contacts
				);
		auto& move_group = model.buildMoveGroup(
				MOVE_GROUP, move_behavior
				);

		std::unique_ptr<UtilityFunction> utility_function;
		switch(config.utility) {
			case UNIFORM:
				utility_function.reset(new UniformUtility);
				break;
			case LINEAR:
				utility_function.reset(new LinearUtility);
				break;
			case INVERSE:
				utility_function.reset(new InverseUtility);
				break;
			case STEP:
				utility_function.reset(new StepUtility);
				break;
		}
		BenchmarkCellFactory cell_factory(*utility_function, config.attractors);
		MooreGrid<BenchmarkCell>::Builder grid(
				cell_factory, config.grid_width, config.grid_height);

		auto local_cells = grid.build(model, {cell_group});
		dump_grid(config.grid_width, config.grid_height, local_cells);

		fpmas::model::UniformGridAgentMapping mapping(
				config.grid_width, config.grid_height,
				config.grid_width * config.grid_height * config.occupation_rate
				);
		fpmas::model::GridAgentBuilder<BenchmarkCell> agent_builder;
		fpmas::model::DefaultSpatialAgentFactory<BenchmarkAgent> agent_factory;

		model.graph().synchronize();
		agent_builder.build(
				model,
				{
				create_relations_neighbors_group, create_relations_contacts_group,
				handle_new_contacts_group, move_group
				},
				agent_factory, mapping);

		// Static node weights
		for(auto cell : cell_group.localAgents())
			cell->node()->setWeight(config.cell_weight);
		for(auto agent : move_group.localAgents())
			agent->node()->setWeight(config.agent_weight);

		model.graph().synchronize();

		scheduler.schedule(0, lb_period, lb_probe.job);
		if(config.agent_interactions == SMALL_WORLD) {
			scheduler.schedule(
					0.20, config.refresh_local_contacts,
					create_relations_neighbors_group.jobs()
					);
			scheduler.schedule(
					0.21, config.refresh_distant_contacts,
					create_relations_contacts_group.jobs()
					);
			scheduler.schedule(
					0.22, config.refresh_distant_contacts,
					handle_new_contacts_group.jobs()
					);
		}
		scheduler.schedule(0.23, 1, move_group.jobs());
		scheduler.schedule(0.24, 1, cell_group.jobs());
		scheduler.schedule(0.3, 1, csv_output.job());

		fpmas::scheduler::TimeStep last_lb_date
			= ((config.num_steps-1) / lb_period) * lb_period;
		// Clears distant nodes
		scheduler.schedule(last_lb_date + 0.01, sync_graph);
		// JSON cell output
		scheduler.schedule(last_lb_date + 0.02, cells_output.job());
		// JSON agent output
		scheduler.schedule(last_lb_date + 0.03, agents_output.job());
}
