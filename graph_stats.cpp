#include "fpmas.h"
#include "metamodel.h"
#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"
#include "output.h"
#include "yaml-cpp/node/parse.h"
#include <algorithm>
#include <cctype>

FPMAS_BASE_DATAPACK_SET_UP(
		GridCell::JsonBase,
		GraphCell::JsonBase,
		MetaGridAgent::JsonBase,
		MetaGridCell::JsonBase,
		MetaGraphCell::JsonBase,
		MetaGraphAgent::JsonBase
		);

FPMAS_BASE_JSON_SET_UP(
		GridCell::JsonBase,
		GraphCell::JsonBase,
		MetaGridAgent::JsonBase,
		MetaGridCell::JsonBase,
		MetaGraphCell::JsonBase,
		MetaGraphAgent::JsonBase
		);

using namespace fpmas::synchro;

int main(int argc, char** argv) {
	FPMAS_REGISTER_AGENT_TYPES(
			GridCell::JsonBase,
			GraphCell::JsonBase,
			MetaGridAgent::JsonBase,
			MetaGridCell::JsonBase,
			MetaGraphCell::JsonBase,
			MetaGraphAgent::JsonBase
			);

	CLI::App app("fpmas-metamodel");
	std::string config_file;
	app.add_option("config_file", config_file, "Graph stats YAML configuration file")
		->required();
	unsigned long seed = fpmas::random::default_seed;
	app.add_option("-s,--seed", seed, "Random seed");

	CLI11_PARSE(app, argc, argv);

	fpmas::seed(seed);
	fpmas::init(argc, argv);
	{
		YAML::Node config = YAML::LoadFile(config_file);
		for(auto env_node : config) {
			GraphConfig graph_config(env_node);

			BasicMetaModelFactory* model_factory;
			switch(graph_config.environment) {
				case Environment::GRID:
					model_factory = new MetaModelFactory<MetaGridModel>;
					break;
				default:
					// All other graph types
					model_factory = new MetaModelFactory<MetaGraphModel>;
			}

			// Building a fake BenchmarkConfig
			BenchmarkConfig benchmark_config(graph_config);
			// No agents
			benchmark_config.occupation_rate = 0.0;
			// 1 step, to launch Zoltan load balancing
			benchmark_config.num_steps = 1;
			// Build cells with a uniform utility, even if unused
			benchmark_config.utility = Utility::UNIFORM;
			// Default cell weight
			benchmark_config.cell_weight = 1.0;

			std::string env_name = env_node["environment"].as<std::string>();
			std::transform(
					env_name.begin(), env_name.end(), env_name.begin(),
					[] (const char& c) {return std::tolower(c);});

			fpmas::scheduler::Scheduler scheduler;
			fpmas::runtime::Runtime runtime(scheduler);
			// Perform a Zoltan load balancing to ensure the stats computing
			// process is balanced
			fpmas::model::ZoltanLoadBalancing zoltan_lb(fpmas::communication::WORLD);
			auto* model = model_factory->build(
					env_name, benchmark_config, scheduler, runtime, zoltan_lb, 1);
			delete model_factory;

			// A DOT output it automatically performed
			model->init()->run();

			// CSV output = Clustering coefficient and characteristic path length
			graph_stats_output(*model, env_name + ".csv");

			delete model;
		}
	}
	fpmas::finalize();
}