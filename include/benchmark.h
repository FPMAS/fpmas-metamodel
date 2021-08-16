#include "fpmas.h"
#include "fpmas/utils/perf.h"
#include "agents.h"

class LoadBalancingProbeTask : public fpmas::api::scheduler::Task {
	private:
		fpmas::api::scheduler::Task& lb_task;
		fpmas::utils::perf::Monitor& monitor;

		fpmas::utils::perf::Probe distribute_probe {"DISTRIBUTE"};

	public:

		LoadBalancingProbeTask(
				fpmas::api::scheduler::Task& lb_task,
				fpmas::utils::perf::Monitor& monitor
				) : lb_task(lb_task), monitor(monitor) {
		}

		void run() override;

}; class LoadBalancingProbe : public fpmas::api::graph::LoadBalancing<fpmas::model::AgentPtr> {
	private:
		fpmas::api::graph::LoadBalancing<fpmas::model::AgentPtr>& lb;

		fpmas::utils::perf::Probe balance_probe {"BALANCE"};

		fpmas::model::detail::LoadBalancingTask lb_task;
		LoadBalancingProbeTask lb_probe_task;

	public:
		fpmas::scheduler::Job job;
		fpmas::utils::perf::Monitor monitor;

		LoadBalancingProbe(
				fpmas::api::model::AgentGraph& graph,
				fpmas::api::graph::LoadBalancing<fpmas::model::AgentPtr>& lb
				) : lb(lb), lb_task(graph, *this), lb_probe_task(lb_task, monitor) {
			job.setBeginTask(lb_probe_task);
		}

		fpmas::graph::PartitionMap balance(
				fpmas::graph::NodeMap<fpmas::model::AgentPtr> node_map,
				fpmas::api::graph::PartitionMode mode) override;

		fpmas::graph::PartitionMap balance(
				fpmas::graph::NodeMap<fpmas::model::AgentPtr> node_map
				) override {
			return this->balance(node_map, fpmas::api::graph::PARTITION);
		}
};

class TestCase;

template<typename T>
class Max {
	public:
		T operator()(const T& a, const T& b) {
			return std::max(a, b);
		}
};

typedef fpmas::io::CsvOutput<
fpmas::scheduler::Date, // Time Step
	unsigned int, // Partitioning time
	unsigned int, // Distribution time
	float, // Local Agents
	float, // Local cells
	float, // Distant agent edges
	float // Distant cell edges
	> LbCsvOutput;

class LoadBalancingCsvOutput :
	public fpmas::io::FileOutput,
	public LbCsvOutput {
		private:
			fpmas::io::FileOutput file;
		public:
			LoadBalancingCsvOutput(TestCase& test_case);
	};

class TestCase {
	private:
		fpmas::model::IdleBehavior cell_behavior;
		fpmas::model::Behavior<BenchmarkAgent> move_behavior {&BenchmarkAgent::move};

	public:
		fpmas::model::GridModel<fpmas::synchro::GhostMode> model;
		std::string lb_algorithm;
		LoadBalancingProbe lb_probe;

	private:
		LoadBalancingCsvOutput csv_output;

	public:
		TestCase(
				std::string lb_algorithm_name,
				std::size_t grid_width, std::size_t grid_height, float occupation_rate,
				fpmas::api::scheduler::Scheduler& scheduler,
				fpmas::api::runtime::Runtime& runtime,
				fpmas::api::model::LoadBalancing& lb_algorithm
				);

		void run() {
			model.runtime().run(100);
		}
};

