#include <math.h>
#include <vector>
#include "readCSV.h"
#include "lineitem.h"
#include "pbbslib/get_time.h"
#include "pbbslib/parse_command_line.h"
#include "pbbslib/collect_reduce.h"
#include "pbbslib/random_shuffle.h"
#include "pbbslib/monoid.h"
#include "pam.h"
#include "utils.h"
#include "tables.h"
//vector<maps> history;
#include "queries.h"
#include "new_orders.h"

//size_t new_lineitem = 0;
//size_t shipped_lineitem = 0;
//size_t keep_versions = 1000000;
//bool if_persistent = false;
//
//double add_new_orders(new_order_entry& no, txn_info& ti) {
//	timer t; t.start();
//	maps m = history[history.size()-1];
//	customer_map cm = m.cm;
//	int num = no.num_items;
//	new_lineitem += num;
//	li_map lm = li_map(no.items, no.items + num);
//	using c_entry = typename customer_map::E;
//	using o_entry = typename order_map::E;
//	o_entry oe = make_pair(no.o.orderkey, make_pair(no.o, lm));
//	o_order_map oom = m.oom;
//	part_to_supp_map psm = m.psm2;
//	order_map om = m.om;
//	supp_to_part_map spm = m.spm2;
//
//
//	{
//		//update cm
//		auto f = [&] (c_entry& e) {
//			order_map o = e.second.second;
//			o.insert(oe);
//			return make_pair(e.second.first, o);
//		};
//		cm.update(no.o.custkey, f);
//
//		//update om
//		om.insert(oe);
//	}
//
//
//
//	{
//		//update supp->part->lineitem
//		for (int i = 0; i < num; i++) {
//			dkey_t suppk = no.items[i].suppkey;
//			dkey_t partk = no.items[i].partkey;
//			int q = no.items[i].quantity();
//			auto f = [&] (supp_to_part_map::E& e) {
//				auto g = [&] (part_supp_and_item_map::E& e2) {
//					Part_supp x = e2.second.first;
//					x.availqty -= q;
//					li_map lm = li_map::insert(e2.second.second, no.items[i]);
//					return make_pair(x, lm);
//				};
//				part_supp_and_item_map t1 = part_supp_and_item_map::update(e.second.second, make_pair(partk,suppk), g);
//				return make_pair(e.second.first, t1);
//			};
//			spm.update(suppk, f);
//		}
//	}
//
//
//	{
//		//update part->supp->lineitem
//		for (int i = 0; i < num; i++) {
//			dkey_t suppk = no.items[i].suppkey;
//			dkey_t partk = no.items[i].partkey;
//			auto f = [&] (part_to_supp_map::E& e) {
//				auto g = [&] (part_supp_and_item_map::E& e2) {
//					li_map lm = li_map::insert(e2.second.second, no.items[i]);
//					return make_pair(e2.second.first, lm);
//				};
//				part_supp_and_item_map t1 = part_supp_and_item_map::update(e.second.second, make_pair(partk, suppk), g);
//				return make_pair(e.second.first, t1);
//			};
//			psm.update(partk, f);
//		}
//	}
//
//	//add to new_order queue
//	ti.new_order_q.push_back(make_pair(no.o.custkey,no.o.orderkey));
//
//	maps nm = m;
//	nm.cm = cm;
//	nm.spm2 = spm;
//	nm.oom = oom;
//	nm.om = om;
//	nm.psm2 = psm;
//	nm.version = history.size();
//
//	history.push_back(nm);
//	double ret_tm = t.stop();
//	return ret_tm;
//}
//
//double payment(payment_entry& pay, txn_info& ti) {
//	maps m = history[history.size()-1];
//	timer t; t.start();
//	customer_map cm = m.cm;
//	auto f = [&] (customer_map::E e) {
//		e.second.first.acctbal -= pay.amount;
//		return e.second;
//	};
//	cm.update(pay.custkey, f);
//	maps nm = m;
//	nm.cm = cm;
//	nm.version = history.size();
//
//	history.push_back(nm);
//	return t.stop();
//}
//
//double shipment(shipment_entry& ship, txn_info& ti) {
//	maps m = history[history.size()-1];
//	timer t; t.start();
//	customer_map cm = m.cm;
//	ship_map sm = m.sm;
//	Date now = ship.date;
//	li_map* li_list = new li_map[ship.num];
//	Date* odate_key = new Date[ship.num];
//	order_map om = m.om;
//	part_to_supp_map psm = m.psm2;
//	supp_to_part_map spm = m.spm2;
//	o_order_map oom = m.oom;
//
//	//update customers
//	for (size_t i = ti.start; i<ti.start+ship.num; i++) {
//		if (i>=ti.new_order_q.size()) break;
//		pair<dkey_t, dkey_t> cokey = ti.new_order_q[i];
//		dkey_t custkey = cokey.first, orderkey = cokey.second;
//		float money = 0.0;
//		auto f = [&] (customer_map::E e) {
//			order_map omf = e.second.second;
//			auto f2 = [&] (order_map::E e) {
//				li_map lm = e.second.second;
//				li_list[i-ti.start] = lm;
//				money = e.second.first.totalprice;
//				odate_key[i-ti.start] = e.second.first.orderdate;
//				auto f3 = [&] (li_map::E e) {
//					//update lineitem's shipdate
//					e.s_date = now;
//					e.set_linestatus();
//					//insert into ship_map
//					sm = ship_map::insert(sm, make_pair(now, e));
//					return e;
//				};
//				e.second.second = li_map::map_set(lm, f3);
//				return e.second;
//			};
//			//increase customer's balance
//			e.second.first.acctbal += money;
//			omf.update(orderkey, f2);
//			e.second.second = omf;
//			return e.second;
//		};
//		cm.update(custkey, f);
//	}
//
//	//update parts and supps
//	for (size_t i = ti.start; i<ti.start+ship.num; i++) {
//		if (i>=ti.new_order_q.size()) break;
//		li_map lm = li_list[i-ti.start];
//		shipped_lineitem += lm.size();
//		auto ff = [&] (li_map::E e, size_t i) {
//			e.s_date = now;
//			e.set_linestatus();
//			dkey_t suppk = e.suppkey;
//			dkey_t partk = e.partkey;
//			//update supp->part->lineitem
//			{
//				auto f = [&] (supp_to_part_map::E& ee) {
//					auto g = [&] (part_supp_and_item_map::E& e2) {
//						li_map lm = li_map::insert(e2.second.second, e);
//						return make_pair(e2.second.first, lm);
//					};
//					part_supp_and_item_map t1 = part_supp_and_item_map::update(ee.second.second, make_pair(partk,suppk), g);
//					return make_pair(ee.second.first, t1);
//				};
//				spm.update(suppk, f);
//			}
//
//
//			//update part->supp->lineitem
//			{
//				auto f = [&] (part_to_supp_map::E& ee) {
//					auto g = [&] (part_supp_and_item_map::E& e2) {
//						li_map lm = li_map::insert(e2.second.second, e);
//						return make_pair(e2.second.first, lm);
//					};
//					part_supp_and_item_map t1 = part_supp_and_item_map::update(ee.second.second, make_pair(partk, suppk), g);
//					return make_pair(ee.second.first, t1);
//				};
//				psm.update(partk, f);
//			}
//
//		};
//		li_map::map_index(lm, ff);
//	}
//
//	//update odate
//	for (size_t i = ti.start; i<ti.start+ship.num; i++) {
//		if (i>=ti.new_order_q.size()) break;
//		pair<dkey_t, dkey_t> cokey = ti.new_order_q[i];
//		dkey_t orderkey = cokey.second;
//		Date dt = odate_key[i-ti.start];
//		auto f = [&] (o_order_map::E e) {
//			order_map om = e.second;
//			auto f2 = [&] (order_map::E e) {
//				li_map lm = e.second.second;
//				li_list[i-ti.start] = lm;
//				auto f3 = [&] (li_map::E e) {
//					//update lineitem's shipdate
//					e.s_date = now;
//					return e;
//				};
//				e.second.second = li_map::map_set(lm, f3);
//				return e.second;
//			};
//			om.update(orderkey, f2);
//			return om;
//		};
//		oom.update(dt, f);
//	}
//
//	ti.start += ship.num;
//	maps nm = m;
//	nm.cm = cm;
//	nm.sm = sm;
//	nm.oom = oom;
//	nm.spm2 = spm;
//	nm.psm2 = psm;
//	nm.version = history.size();
//
//	history.push_back(nm);
//	delete[] li_list;
//	delete[] odate_key;
//	return t.stop();
//}
//
pair<double, double> geo_mean(double* x, int round) {
	double res = 0.0, dev = 0.0;
	for (int j = 0; j < round; j++) {
		double tmp = log2(x[j]*1000.0);
		res += tmp;
	}
	res = res/(round+0.0);
	res = exp2(res);
	double m = 0.0;
	for (int j = 0; j < round; j++) {
		double tmp = log2(x[j]*1000.0/res);
		m += tmp*tmp;

	}
	dev = exp2(sqrt(m/(round+0.0)));
	return make_pair(res, dev);
}

pair<double, double> arith_mean(double* x, int round) {
    double res = 0.0, dev = 0.0;
    for (int j = 0; j < round; j++) {
        double tmp = x[j]*1000;
        res += tmp;
    }
    res = res/(round+0.0);
    double m = 0.0;
    for (int j = 0; j < round; j++) {
        double tmp = (x[j]*1000)/res;
        m += tmp*tmp;

    }
    dev = sqrt(m/(round+0.0));
    return make_pair(res, dev);
}

//void output_new_order(new_order_entry t, ofstream& myfile) {
//	myfile << "N|" << t.num_items <<
//	"|" << t.o.orderkey <<
//	"|" << t.o.custkey <<
//	"|" << t.o.orderdate <<
//	"|" << t.o.shippriority <<
//	"|" << t.o.orderpriority <<
//	"|" << t.o.status <<
//	"|" << t.o.strings << endl;
//	for (int i = 0; i < t.num_items; i++) {
//		myfile << t.items[i].partkey <<
//		"|" << t.items[i].suppkey <<
//		"|" << t.items[i].c_date <<
//		"|" << t.items[i].r_date <<
//		"|" << t.items[i].e_price <<
//		"|" << t.items[i].discount.val() <<
//		"|" << t.items[i].tax.val() << endl;
//	}
//}
//
//void output_payment(payment_entry t, ofstream& myfile) {
//	myfile << "P|" << t.custkey << "|" << t.amount << endl;
//}
//
//void output_shipment(shipment_entry t, ofstream& myfile) {
//	myfile << "S|" << t.date << "|" << t.num << endl;
//}
//
//
//double exe_txns(transaction* txns, int num_txns, txn_info& ti, bool verbose) {
//	cout << "start transactions" << endl;
//	int numn = 0, nump = 0, nums = 0;
//	double* n_timer = new double[num_txns];
//	double* p_timer = new double[num_txns];
//	double* s_timer = new double[num_txns];
//
//	cout << "initial_lineitem: " << li_map::GC::used_node() << endl;
//    cout << "initial_part_supp: " << part_supp_and_item_map::GC::used_node() << endl;
//    cout << "initial_part: " << part_to_supp_map::GC::used_node() << endl;
//	cout << "initial_order: " << order_map::GC::used_node() << endl;
//	cout << "initial_customer: " << customer_map::GC::used_node() << endl;
//
//	ofstream myfile;
//    myfile.open ("logs/tpcc.log");
//
//	timer tm; tm.start();
//    for (int i = 0; i < num_txns; i++) {
//      if (txns[i].type == 'N') {
//		  double t = add_new_orders(ti.new_orders[i], ti);
//		  if (if_persistent) output_new_order(ti.new_orders[i], myfile);
//		  n_timer[numn++] = t;
//	  }
//	  if (txns[i].type == 'P') {
//		  double t = payment(ti.payments[i], ti);
//		  if (if_persistent) output_payment(ti.payments[i], myfile);
//		  p_timer[nump++] = t;
//	  }
//	  if (txns[i].type == 'S') {
//		  double t = shipment(ti.shipments[i], ti);
//		  if (if_persistent) output_shipment(ti.shipments[i], myfile);
//		  s_timer[nums++] = t;
//	  }
//	  if (if_collect && (history.size() >= keep_versions)) history[history.size()-keep_versions].clear();
//	  size_t t = li_map::GC::used_node();
//	  if (t > max_lineitem) max_lineitem = t;
//	  t = part_supp_and_item_map::GC::used_node();
//	  if (t > max_part_supp) max_part_supp = t;
//	  t = part_to_supp_map::GC::used_node();
//	  if (t > max_part) max_part = t;
//	  t = order_map::GC::used_node();
//	  if (t > max_order) max_order = t;
//	  t = customer_map::GC::used_node();
//	  if (t > max_customer) max_customer = t;
//    }
//    if (verbose) cout << endl;
//	finish = true;
//	double ret = tm.stop();
//	cout << "total txns: " << ret << endl;
//	myfile.close();
//
//	pair<double, double> x1 = arith_mean(n_timer, numn);
//	pair<double, double> x2 = arith_mean(p_timer, nump);
//	pair<double, double> x3 = arith_mean(s_timer, nums);
//
//	cout << "new_order: " << numn << " " << x1.first << "dev: " << x1.second << endl;
//	cout << "payment: " << nump << " " << x2.first << "dev: " << x2.second << endl;
//	cout << "shipment: " << nums << " " << x3.first << "dev: " << x3.second << endl;
//
//    cout << "max_lineitem: " << max_lineitem << endl;
//    cout << "max_part_supp: " << max_part_supp << endl;
//    cout << "max_part: " << max_part << endl;
//	cout << "max_order: " << order_map::GC::used_node() << endl;
//	cout << "max_customer: " << customer_map::GC::used_node() << endl;
//
//	return ret;
//}
//
//void post_process(double** tm, int round) {
//  double res[queries];
//  double tot = 0;
//  int no[] = {22,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21};
//  for (int i = 0; i < queries; i++) {
//	res[i] = 0;
//	cout << "processing query " << no[i] << endl;
//	for (int j = 0; j < round; j++) {
//		double tmp = log2(tm[i][j]*1000.0);
//		res[i] += tmp;
//		cout << tm[i][j] << " ";
//	}
//	cout << endl;
//	res[i] = res[i]/(round+0.0);
//	tot += res[i];
//	res[i] = exp2(res[i]);
//	double m = 0.0, dev = 0.0;
//	for (int j = 0; j < round; j++) {
//		double tmp = log2(tm[i][j]*1000.0/res[i]);
//		m += tmp*tmp;
//	}
//	dev = exp2(sqrt(m/(round+0.0)));
//	cout << "geo mean of query " << no[i] << " : " << res[i] << ", sdev: " << dev << endl << endl;
//
//  }
//  cout << "geo mean of all: " << exp2(tot/(queries+0.0)) << endl;
//}
//
//void test_all(bool verbose, bool if_query, bool if_update,
//	      int scale, int num_txns, string data_directory) {
//
//  history.push_back(make_maps(data_directory, verbose));
//  memory_stats();
//  if (verbose) nextTime("gc for make maps");
//
//  //prepare txns
//  txn_info ti;
//
//  ti.new_order_q.reserve(num_txns);
//  ti.start = 0;
//
//  ti.new_orders = generate_new_orders(num_txns, history[0]);
//  ti.payments = generate_payments(num_txns, history[0]);
//  ti.shipments = generate_shipments(num_txns, ti.new_orders);
//  transaction* txns = new transaction[num_txns];
//
//  int wgts[] = {45, 88, 92, 96, 100};
//  //int wgts[] = {100, 100, 100};
//  for (int i = 0; i < num_txns; i++) {
//      int rand = random_hash('q'+'t', i, 0, 92);
//      if (rand<wgts[0]) {
//          txns[i].type = 'N'; txns[i].ind = i;
//      } else {
//          if (rand<wgts[1]) {
//              txns[i].type = 'P'; txns[i].ind = i;
//          } else if (rand < wgts[2]) {
//              txns[i].type = 'S'; txns[i].ind = i;
//          }
//      }
//  }
//  nextTime("generate txns");
//
//  if (verbose) {
//    memory_stats();
//    cout << "Lineitem size: " << sizeof(Lineitem) << endl;
//    cout << "Order size: " << sizeof(Orders) << endl;
//    cout << "Customer size: " << sizeof(Customer) << endl;
//    cout << "Part supplier size: " <<  sizeof(Part_supp) << endl;
//  }
//  if (if_query && if_update) {
//      double** tm; int round = 0;
//      tm = new double*[queries];
//      for (int i = 0; i < queries; i++) {
//          tm[i] = new double[100];
//      }
//	  memory_stats();
//      nextTime("ready");
//
//      par_do([&] () {exe_txns(txns, num_txns, ti, verbose);},
//	     [&] () {exe_query(verbose, tm, round);});
//
//      nextTime("query and update");
//      post_process(tm, round);
//	  memory_stats();
//	  cout << "new: " << new_lineitem << endl;
//	  cout << "shipped: " << shipped_lineitem << endl;
//  } else {
//    if (if_query) {
//        double** tm; int round = 0;
//        tm = new double*[queries];
//        for (int i = 0; i < queries; i++) {
//          tm[i] = new double[100];
//        }
//        exe_query(verbose, tm, round, rpt);
//        post_process(tm, round);
//    }
//    if (if_update) exe_txns(txns, num_txns, ti, verbose);
//    nextTime("query or update");
//  }
//}
void output_res(double** tm, int round, int queries) {
    double tot = 0;
    double res[queries];
    for (int i = 0; i < queries; i++) {
        res[i] = 0;
        cout << "processing query " << i+1 << endl;
        for (int j = 0; j < round; j++) {
            double tmp = log2(tm[i][j]*1000.0);
            res[i] += tmp;
            cout << tm[i][j] << " ";
        }
        cout << endl;
        res[i] = res[i]/(round+0.0);
        tot += res[i];
        res[i] = exp2(res[i]);
        double m = 0.0, dev = 0.0;
        for (int j = 0; j < round; j++) {
            double tmp = log2(tm[i][j]*1000.0/res[i]);
            m += tmp*tmp;
        }
        dev = exp2(sqrt(m/(round+0.0)));
        cout << "geo mean of query " << i+1 << " : " << res[i] << ", sdev: " << dev << endl << endl;
    }
}

# include "Q1.h"
# include "Q2.h"
# include "Q3.h"
# include "Q4.h"
# include "Q5.h"
# include "Q6.h"
# include "Q7.h"
# include "Q8.h"
# include "Q9.h"
# include "Q10.h"
# include "Q11.h"
# include "Q12.h"
# include "Q13.h"
# include "Q14.h"
# include "Q15.h"
# include "Q16.h"
# include "Q17.h"
# include "Q18.h"
# include "Q19.h"
# include "Q20.h"

void test(string data_directory, bool verbose) {
    maps m = make_maps_test(data_directory, verbose);
    int queries = 22, round = 6;
    double** tm;
    tm = new double*[queries];
    for (int i = 0; i < queries; i++) {
      tm[i] = new double[100];
    }
    for (int i = 0; i < round; i++) {
        tm[0][i] = Q1time(m, verbose);
        tm[1][i] = Q2time(m, verbose);
        tm[2][i] = Q3time(m, verbose);
        tm[3][i] = Q4time(m, verbose);
        tm[4][i] = Q5time(m, verbose);
        tm[5][i] = Q6time(m, verbose);
        tm[6][i] = Q7time(m, verbose);
        tm[7][i] = Q8time(m, verbose);
        tm[8][i] = Q9time(m, verbose);
        tm[9][i] = Q10time(m, verbose);
        tm[10][i] = Q11time(m, verbose);
        tm[11][i] = Q12time(m, verbose);
        tm[12][i] = Q13time(m, verbose);
        tm[13][i] = Q14time(m, verbose);
        tm[14][i] = Q15time(m, verbose);
        tm[15][i] = Q16time(m, verbose);
        tm[16][i] = Q17time(m, verbose);
        tm[17][i] = Q18time(m, verbose);
        tm[18][i] = Q19time(m, verbose);
        tm[19][i] = Q20time(m, verbose);
    }

    output_res(tm, round, queries);
}

int main(int argc, char **argv) {
    commandLine P(argc, argv, "./test [-v] [-q] [-u] [-c] [-s size] [-t txns] [-d directory] [-y keep_versions] [-p]");
    bool verbose = P.getOption("-v");
    bool if_query = P.getOption("-q");
    bool if_update = P.getOption("-u");
//  if_persistent = P.getOption("-p");
//  keep_versions = P.getOptionIntValue("-y", 1000000);
//  if_collect = P.getOption("-c");
    string default_directory = "/ssd1/tpch/S10/";
    int scale = P.getOptionIntValue("-s", 10);
    int num_txns = P.getOptionIntValue("-t", 10000);
    if (scale == 100) default_directory = "/ssd1/tpch/S100/";
    if (scale == 1) default_directory = "/ssd1/tpch/S1/";

    string data_directory = P.getOptionValue("-d", default_directory);

//  test_all(verbose, if_query, if_update,
//	   scale, num_txns, data_directory);

    memory_stats();

    test(data_directory, verbose);
    return 0;
}
