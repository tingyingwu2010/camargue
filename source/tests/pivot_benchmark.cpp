#include "config.hpp"

#ifdef CMR_DO_TESTS

#include "separator.hpp"
#include "lp_interface.hpp"
#include "core_lp.hpp"
#include "datagroups.hpp"
#include "util.hpp"

#include <catch.hpp>

#include <algorithm>
#include <array>
#include <functional>
#include <vector>
#include <string>
#include <iostream>
#include <iostream>

#include <cmath>

using std::array;
using std::function;
using std::min;
using std::max;
using std::abs;
using std::vector;

using std::string;
using std::cout;
using std::endl;
using std::setprecision;

template <typename T>
using Triple = std::array<T, 3>;

static void core_nd(CMR::LP::CoreLP &core) { core.primal_pivot(); }
static void core_itlim(CMR::LP::CoreLP &core)
{
    double objval = core.active_tourlen();
    while (objval >= core.active_tourlen()) {
        core.one_primal_pivot();
        objval = core.get_objval();
    }
}

using ImplPair = std::pair<string, function<void(CMR::LP::CoreLP &)>>;
array<ImplPair, 2> piv_impls{ImplPair("low lim", core_nd),
    ImplPair("it lim", core_itlim)};


using ProbPair = std::pair<string, vector<double>>;

static vector<ProbPair> impl_benchmarks {
    ProbPair("d2103", vector<double>(8, 1.0)),
    ProbPair("u2152", vector<double>(8, 1.0)),
    ProbPair("u2319", vector<double>(8, 1.0)),
    ProbPair("pr2392", vector<double>(8, 1.0)),
     ProbPair("pcb3038", vector<double>(8, 1.0)),
    ProbPair("fl3795", vector<double>(8, 1.0)),
    ProbPair("fnl4461", vector<double>(8, 1.0)),
    ProbPair("rl5915", vector<double>(8, 1.0)),
    ProbPair("rl5934", vector<double>(8, 1.0)),
    ProbPair("pla7397", vector<double>(8, 1.0)),
    ProbPair("rl11849", vector<double>(8, 1.0)),
    ProbPair("usa13509", vector<double>(8, 1.0)),
    ProbPair("brd14051", vector<double>(8, 1.0)),
};


using RepTuple = std::tuple<string, Triple<int>, Triple<double>, int>;

static vector<RepTuple> table_entries{
    RepTuple("d2103", {{0,0,0}}, {{1.0, 1.0, 1.0}}, 2103),
    RepTuple("fl3795", {{0,0,0}}, {{1.0, 1.0, 1.0}}, 3795),
    RepTuple("fnl4461", {{0,0,0}}, {{1.0, 1.0, 1.0}}, 4461),
    RepTuple("pcb3038", {{0,0,0}}, {{1.0, 1.0, 1.0}}, 3038),
    RepTuple("pla7397", {{0,0,0}}, {{1.0, 1.0, 1.0}}, 7397),
    RepTuple("pr2392", {{0,0,0}}, {{1.0, 1.0, 1.0}}, 2392),
    RepTuple("rl5915", {{0,0,0}}, {{1.0, 1.0, 1.0}}, 5915),
    RepTuple("rl5934", {{0,0,0}}, {{1.0, 1.0, 1.0}}, 5934),
    RepTuple("u2152", {{0,0,0}}, {{1.0, 1.0, 1.0}}, 2152),
    RepTuple("u2319", {{0,0,0}}, {{1.0, 1.0, 1.0}}, 2319),
    RepTuple("brd14051", {{0,0,0}}, {{1.0, 1.0, 1.0}}, 14051),
    RepTuple("rl11849", {{0,0,0}}, {{1.0, 1.0, 1.0}}, 11849),
    RepTuple("usa13509", {{0,0,0}}, {{1.0, 1.0, 1.0}}, 13509),
    };

//this test uses some pretty heinous array indexing tricks
SCENARIO ("Comparing pivot protocols with round of cuts",
          "[.LP][.Sep][.one_primal_pivot][.nondegen_pivot][.benchmark]"
          "[.table][.primops-piv-piv]") {
    using namespace CMR;

    for (ProbPair &pp : impl_benchmarks) {
    string &prob = pp.first;
    vector<double> &data = pp.second;
    for (int trial : {1, 2, 3, 4, 5}) {
    GIVEN ("A core LP for " + prob + " trial " + std::to_string(trial)) {
        Data::Instance inst("problems/" + prob + ".tsp", 99);
        Graph::CoreGraph core_graph(inst);
        Data::BestGroup b_dat(inst, core_graph,
                              "test_data/tours/" + prob + ".sol");
        LP::CoreLP core(core_graph, b_dat);
        for (int i = 0; i < 2; ++i) {
            string &impl_name = piv_impls[i].first;
            auto impl_func = piv_impls[i].second;
            int start_ind = 4 * i;
            THEN ("We can pivot and add cuts with impl " + impl_name) {
                double deg_piv_t = util::zeit();
                impl_func(core);
                deg_piv_t = util::zeit() - deg_piv_t;
                double deg_piv_val = core.get_objval();

                cout << "Degree pivot with " << impl_name << "\n\t"
                     << deg_piv_t << "s\tobjval "
                     << deg_piv_val << "\n";
                data[start_ind++] *= deg_piv_t;
                data[start_ind++] = deg_piv_val;

                int ncount = inst.node_count();
                auto piv_x = core.lp_vec();

                vector<int> island;
                Data::SupportGroup s_dat(core_graph.get_edges(),
                                         piv_x, island, ncount);

                Data::KarpPartition kpart;

                Sep::Separator sep(core_graph.get_edges(),
                                   core.get_active_tour(), s_dat, kpart, 99);

                core.pivot_back(false);

                int numrows = core.num_rows();

                if (sep.connect_sep())
                    core.add_cuts(sep.connect_cuts_q());
                if (sep.segment_sep())
                    core.add_cuts(sep.segment_q());
                if (sep.fast2m_sep())
                    core.add_cuts(sep.fastblossom_q());
                if (sep.blkcomb_sep())
                    core.add_cuts(sep.blockcomb_q());

                cout << "Found " << (core.num_rows() - numrows)
                     << " cuts\n";

                double cut_piv_t = util::zeit();
                impl_func(core);
                cut_piv_t = util::zeit() - cut_piv_t;
                double cut_piv_val = core.get_objval();

                data[start_ind++] *= cut_piv_t;
                data[start_ind++] = cut_piv_val;

                cout << "Cut pivot with " << impl_name << "\n\t"
                     << cut_piv_t << "s\tobjval "
                     << cut_piv_val << "\n";

            }
        }
        cout << "\n====\n";
    }
    }
    }

    THEN ("Report the data") {
        cout << "Instance\tProtocol\tDegree Pivot\t\tCuts Pivot\n";
        cout << "\tCPU time\tValue\tCPU time\tValue\n";
        for (ProbPair &pp : impl_benchmarks) {
            cout << pp.first;
            for (int i = 0; i < 2; ++i) {
                string impl_name = piv_impls[i].first;
                vector<double> dat_range(pp.second.begin() + (4 * i),
                                         pp.second.begin() + 4 + (4 * i));
                cout << "\t" << impl_name << "\t";
                for (int i = 0; i < 4; ++i)
                    if (i % 2)
                        printf("%.2f\t", dat_range[i]);
                    else
                        printf("%.6f\t",
                               std::pow(dat_range[i], 1.0/5.0));
                cout << "\n";
            }
        }
    }

}

SCENARIO ("Comparing pivot protocols as optimizers",
          "[.LP][.Relaxation][.single_pivot][.nondegen_pivot][.benchmark]"
          "[.table][.primops-piv-opt]") {
    using namespace CMR;
    namespace Eps = Epsilon;

    for (RepTuple &te : table_entries) {
        string prob = std::get<0>(te);
        Triple<int> &piv_counts = std::get<1>(te);
        Triple<double> &piv_times = std::get<2>(te);
         int ncount = std::get<3>(te);
    for (int trial : {1, 2, 3, 4, 5}) {
        GIVEN ("The degree LP for " + prob + ", trial " +
               std::to_string(trial)) {
            Data::Instance inst("problems/" + prob + ".tsp", 99);
            Graph::CoreGraph core_graph(inst);
            Data::BestGroup b_dat(inst, core_graph,
                                  "test_data/tours/" + prob + ".sol");
            LP::CoreLP core(core_graph, b_dat);

            double tourlen = b_dat.min_tour_value;
            REQUIRE(core.get_objval() == tourlen);

            THEN ("We can primal opt the degree LP") {
                cout << "Testing primal opt..." << endl;
                double popt = util::zeit();
                core.primal_opt();
                popt = util::zeit() - popt;
                piv_times[0] *= popt;
                cout << "Did primal opt in " << popt << "s\n";
                piv_counts[0] = core.it_count();
            }

            THEN ("We can primal opt one nd pivot at a time") {
                cout << "Testing nd opt..." << endl;
                double ndt = util::zeit();
                int nd_itcount = 0;
                while (!core.dual_feas()) {
                    double objval = core.get_objval();
                    core.nondegen_pivot(objval - Eps::Zero);
                    nd_itcount += core.it_count();
                }
                ndt = util::zeit() - ndt;
                piv_times[1] *= ndt;
                cout << "Did nd opt in " << ndt << "s\n";
                piv_counts[1] = nd_itcount;
            }

            THEN ("We can primal opt a single pivot at a time") {
                cout << "Testing itlim opt..." << endl;
                double itt = util::zeit();
                int it_itcount = 0;
                while (!core.dual_feas()) {
                    ++it_itcount;
                    core.one_primal_pivot();
                }
                itt = util::zeit() - itt;
                piv_times[2] *= itt;
                cout << "Did itlim opt in " << itt << "s\n";
                piv_counts[2] = it_itcount;
            }
        }
    }
    }
    cout << "\n";

    THEN ("Report the results") {
        std::sort(table_entries.begin(), table_entries.end(),
                  [](RepTuple r, RepTuple t)
                  { return std::get<3>(r) < std::get<3>(t); });

        cout << "Instance\tProtocol\tCPU Time (scaled)"
             <<"\tIteration Count" << endl;
        for (RepTuple &te : table_entries) {
            string prob = std::get<0>(te);
            Triple<int> &piv_counts = std::get<1>(te);
            Triple<double> &piv_times = std::get<2>(te);
            using DescPair = std::pair<string, int>;
            std::vector<DescPair>
            reports{DescPair("Primal opt", 0),
                DescPair("Nondeg opt", 1),
                DescPair("Itlim opt", 2)};
            double base_scale = std::pow(piv_times[0], 1.0/5.0);
            cout << prob;
            for (auto dp : reports) {
                string protocol = dp.first;
                int index = dp.second;
                printf("\t%s\t%.2f\t%d\n", protocol.c_str(),
                       std::pow(piv_times[index], 1.0/5.0) / base_scale,
                       piv_counts[index]);
            }
        }
    }
}

#endif
