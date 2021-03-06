#include "fpmas.h"
#include "benchmark.h"
#include "fpmas/model/spatial/cell_load_balancing.h"

FPMAS_BASE_DATAPACK_SET_UP(
		GridCell::JsonBase,
		BenchmarkAgent::JsonBase,
		BenchmarkCell::JsonBase
		);

FPMAS_BASE_JSON_SET_UP(
		GridCell::JsonBase,
		BenchmarkAgent::JsonBase,
		BenchmarkCell::JsonBase
		);

int main(int argc, char** argv) {
	FPMAS_REGISTER_AGENT_TYPES(
			GridCell::JsonBase,
			BenchmarkAgent::JsonBase,
			BenchmarkCell::JsonBase
			);
	if(argc <= 1) {
		std::cerr << "[FATAL ERROR] Missing config file argument" << std::endl;
		return EXIT_FAILURE;
	}

	fpmas::init(argc, argv);
	{
		BenchmarkConfig config(argv[1]);
		if(!config.is_valid)
			return EXIT_FAILURE;

		for(auto test_case : config.test_cases) {
			for(auto lb_period : test_case.lb_periods) {
				fpmas::scheduler::Scheduler scheduler;
				fpmas::runtime::Runtime runtime(scheduler);

				switch(test_case.algorithm) {
					case ZOLTAN_LB:
						{
							ZoltanLoadBalancing zoltan_lb(
									fpmas::communication::WORLD, lb_period);
							TestCase(
									"zoltan_lb", config,
									scheduler, runtime, zoltan_lb, lb_period
									).run();
						}
						break;
					case SCHEDULED_LB:
						{
							ZoltanLoadBalancing zoltan_lb(
									fpmas::communication::WORLD, lb_period);
							ScheduledLoadBalancing scheduled_load_balancing(
									zoltan_lb, scheduler, runtime
									);
							TestCase(
									"scheduled_lb", config,
									scheduler, runtime, scheduled_load_balancing,
									lb_period
									).run();
						}
						break;
					case GRID_LB:
						{
							GridLoadBalancing grid_lb(
									config.grid_width, config.grid_height,
									fpmas::communication::WORLD
									);
							TestCase(
									"grid_lb", config,
									scheduler, runtime, grid_lb, lb_period
									).run();
						}
						break;
					case ZOLTAN_CELL_LB:
						{
							ZoltanLoadBalancing zoltan_lb(
									fpmas::communication::WORLD, lb_period);
							CellLoadBalancing zoltan_cell_lb(
									fpmas::communication::WORLD, zoltan_lb
									);
							TestCase(
									"zoltan_cell_lb", config,
									scheduler, runtime, zoltan_cell_lb, lb_period
									).run();
						}
						break;
					case RANDOM_LB:
						{

							RandomLoadBalancing random_lb(fpmas::communication::WORLD);
							TestCase(
									"random_lb", config,
									scheduler, runtime, random_lb, lb_period
									).run();
						}
						break;
				}
			}
		}
	}
	fpmas::finalize();
}
