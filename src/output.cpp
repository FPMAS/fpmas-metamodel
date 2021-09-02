#include "benchmark.h"

void dump_grid(
		std::size_t grid_width, std::size_t grid_height,
		std::vector<BenchmarkCell*> local_cells) {
	typedef std::pair<fpmas::model::DiscretePoint, float> CellView;
	std::vector<CellView> local_cells_view;
	for(auto cell : local_cells)
		local_cells_view.push_back({cell->location(), cell->getUtility()});

	fpmas::communication::TypedMpi<std::vector<CellView>> cell_mpi(fpmas::communication::WORLD);
	auto global_cells = fpmas::communication::reduce(
			cell_mpi, 0, local_cells_view, fpmas::utils::Concat()
			);
	FPMAS_ON_PROC(fpmas::communication::WORLD, 0) {
		fpmas::io::FileOutput grid_file {"grid.json"};
		std::vector<std::vector<float>> grid;
		grid.resize(grid_height);
		for(auto& row : grid)
			row.resize(grid_width);

		for(auto cell : global_cells)
			grid[cell.first.y][cell.first.x] = cell.second;

		fpmas::io::JsonOutput<decltype(grid)>(grid_file, [&grid] () {return grid;})
			.dump();
	}
}

LoadBalancingCsvOutput::LoadBalancingCsvOutput(TestCase& test_case)
	:
		fpmas::io::FileOutput(test_case.lb_algorithm_name + ".%r.csv", test_case.model.getMpiCommunicator().getRank()),
		LbCsvOutput(*this,
			{"TIME", [&test_case] {return test_case.model.runtime().currentDate();}},
			{"BALANCE_TIME", [&test_case] {
			auto result = std::chrono::duration_cast<std::chrono::microseconds>(
					test_case.lb_probe.monitor.totalDuration("BALANCE")
					).count();
			return result;
			}},
			{"DISTRIBUTE_TIME", [&test_case] {
			auto result = std::chrono::duration_cast<std::chrono::microseconds>(
					test_case.lb_probe.monitor.totalDuration("DISTRIBUTE")
					).count();
			test_case.lb_probe.monitor.clear();
			return result;
			}},
			{"AGENTS", [&test_case] {
			float total_weight = 0;
			for(auto agent : test_case.model.getGroup(1).localAgents())
				total_weight += agent->node()->getWeight();
			return total_weight;
			}},
			{"CELLS", [&test_case] {
			float total_weight = 0;
			for(auto agent : test_case.model.getGroup(0).localAgents())
				total_weight += agent->node()->getWeight();
			return total_weight;
			}},
			{"DISTANT_AGENT_EDGES", [&test_case] {
			float total_weight = 0;
			for(auto agent : test_case.model.getGroup(1).localAgents())
				for(auto edge : agent->node()->getOutgoingEdges())
					if(edge->state() == fpmas::api::graph::DISTANT)
						total_weight+=edge->getWeight();
			return total_weight;
			}},
			{"DISTANT_CELL_EDGES", [&test_case] {
			float total_weight = 0;
			for(auto cell : test_case.model.getGroup(0).localAgents())
				for(auto edge : cell->node()->getOutgoingEdges())
					if(edge->state() == fpmas::api::graph::DISTANT)
						total_weight+=edge->getWeight();
			return total_weight;
			}}
) {
}


void CellsOutput::dump() {
	auto cells = gather_cells();
	FPMAS_ON_PROC(model.getMpiCommunicator(), 0) {
		nlohmann::json j = cells;
		output_file.get() << j.dump();
	}
}

std::vector<std::vector<int>> CellsOutput::gather_cells() {
	typedef std::pair<fpmas::model::DiscretePoint, int> CellLocation;

	std::vector<fpmas::api::model::Agent*> local_cells = model.getGroup(0).localAgents();
	std::vector<CellLocation> local_cells_location;
	for(auto cell : local_cells)
		local_cells_location.push_back({
				dynamic_cast<BenchmarkCell*>(cell)->location(),
				cell->node()->location()
				});

	fpmas::communication::TypedMpi<std::vector<CellLocation>> cell_mpi(fpmas::communication::WORLD);
	auto global_cells = fpmas::communication::reduce(
			cell_mpi, 0, local_cells_location, fpmas::utils::Concat()
			);

	std::vector<std::vector<int>> grid;
	FPMAS_ON_PROC(fpmas::communication::WORLD, 0) {
		fpmas::io::FileOutput cells_file {"cells.json"};
		grid.resize(grid_height);
		for(auto& row : grid)
			row.resize(grid_width);

		for(auto cell : global_cells)
			grid[cell.first.y][cell.first.x] = cell.second;

		fpmas::io::JsonOutput<decltype(grid)>(cells_file, [&grid] () {return grid;})
			.dump();
	}
	return grid;
}

std::vector<std::vector<std::size_t>> AgentsOutput::gather_agents() {
	std::vector<std::vector<std::size_t>> grid;
	grid.resize(grid_height);
	for(auto& row : grid)
		row.resize(grid_width, 0);

	for(auto agent : model.getGroup(1).localAgents()) {
		auto grid_agent = dynamic_cast<BenchmarkAgent*>(agent);
		grid[grid_agent->locationPoint().y][grid_agent->locationPoint().x]++;
	}
	return grid;
}
