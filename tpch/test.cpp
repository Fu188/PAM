#include "test.h"

maps m;
std::map <int64_t, vector<insert_form>> ready_insert;
std::map<int64_t, OrderLine*> orderline_object;
std::map<int64_t, pair<int, int>> orderline_cnt;

void Vec2Map(vector <insert_form> v, std::map<string, string> &row_map) {
    for (int i = 0; i < v.size(); i++) {
        row_map[v[i].col_name] = v[i].value;
    }
}

double insertOrder(int64_t txn_id, vector <insert_form> v) {
    timer t; t.start();
    std::map <string, string> row_map;
    Vec2Map(v, row_map);
    uint o_id = stoi(row_map["o_id"]);
    uint o_d_id = stoi(row_map["o_d_id"]);
    uint o_w_id = stoi(row_map["o_w_id"]);
    uint o_c_id = stoi(row_map["o_c_id"]);
    Date o_entry_d = Date(row_map["o_entry_d"]);
    uint o_carrier_id = stoi(row_map["o_carrier_id"]);
    uint o_ol_cnt = stoi(row_map["o_ol_cnt"]);
    // TRICKS: o_c_pkey = 100000*o_w_id + 10000* o_d_id + o_c_id;
    // TRICKS: o_pkey = 100000*o_w_id + 10000* o_d_id + o_id;
    uint o_pkey = 100000*o_w_id + 10000* o_d_id + o_id;
    uint o_c_pkey = 100000*o_w_id + 10000* o_d_id + o_c_id;
    Order order = Order{o_id, o_d_id, o_w_id, o_c_id, o_entry_d, o_carrier_id, o_ol_cnt, o_c_pkey, o_pkey};
    using c_entry = typename customer_map::E;
	using o_entry = typename order_map::E;
	customer_map cm = m.cm;
	o_order_map oom = m.oom;
	order_map om = m.om;
    OrderLine* orderline;

    // insert order first, orderline is NULL
    if (orderline_object.find(txn_id) == orderline_object.end()) {
        orderline = new OrderLine[o_ol_cnt];
        orderline_object.insert(make_pair(txn_id, orderline));
        orderline_cnt.insert(make_pair(txn_id,make_pair(0, o_ol_cnt)));
    }
    // insert orderline first, orderline exists
    else {
        orderline = orderline_object.find(txn_id)->second;
        orderline_cnt.find(txn_id)->second.second = o_ol_cnt;
    }

    ol_map ol_m = ol_map(orderline, orderline+o_ol_cnt);
    o_entry oe = make_pair(o_pkey, make_pair(order, ol_m));

    // update cm
    auto f = [&] (c_entry& e) {
        order_map o = e.second.second;
        o.insert(oe);
        return make_pair(e.second.first, o);
    };
    cm.update(o_c_pkey, f);

    // update om
    om.insert(oe);

    m.cm = cm;
    m.om = om;
    m.oom = oom;

    return t.stop();
}

double insertOrderline(int64_t txn_id, vector<insert_form> v) {
    timer t; t.start();
    std::map <string, string> row_map;
    Vec2Map(v, row_map);
    uint ol_o_id = stoi(row_map["ol_o_id"]);
    uint ol_d_id = stoi(row_map["ol_d_id"]);
    uint ol_w_id = stoi(row_map["ol_w_id"]);
    uint ol_number = stoi(row_map["ol_number"]);
    uint ol_i_id = stoi(row_map["ol_i_id"]);
    uint ol_supply_w_id = stoi(row_map["ol_supply_w_id"]);
    Date ol_delivery_d = Date(row_map["ol_delivery_d"]);
    uint ol_quantity = stoi(row_map["ol_quantity"]);
    double ol_amount = stof(row_map["ol_amount"]);
    // TRICKS: ol_suppkey = (ol_i_id*ol_w_id)%10000;
    // TRICKS: ol_pkey = 100000*ol_w_id + 10000* ol_d_id + ol_o_id;
    uint ol_suppkey = (ol_i_id*ol_w_id)%10000;
    uint ol_pkey = 100000*ol_w_id + 10000* ol_d_id + ol_o_id;
    OrderLine ol = OrderLine{ol_o_id, ol_d_id, ol_w_id, ol_number, ol_i_id, ol_supply_w_id, ol_delivery_d, ol_quantity, ol_amount, ol_suppkey, ol_pkey};
    delivery_map dm = m.dm;
    item_to_supp_map ism = m.ism2;
    supp_to_item_map sim = m.sim2;
    supp_to_item_map_tmp sim_tmp = m.sim2_2;
    OrderLine *orderline;

    // insert order first, orderline is NULL
    if (orderline_object.find(txn_id) == orderline_object.end()) {
        // 15 is the max orderline number (decided by data generator)
        orderline = new OrderLine[15];
        orderline_object.insert(make_pair(txn_id, orderline));
        orderline_cnt.insert(make_pair(txn_id,make_pair(1, 0)));
    }
    // insert orderline first, orderline exists
    else {
        orderline = orderline_object.find(txn_id)->second;
        orderline_cnt.find(txn_id)->second.first ++;
        if (orderline_cnt.find(txn_id)->second.first == orderline_cnt.find(txn_id)->second.second) {
            orderline_cnt.erase(txn_id);
            orderline_object.erase(txn_id);
        }
    }

    orderline[ol_number-1] = ol;

    {
        //update supp->part->lineitem
        auto f = [&] (supp_to_item_map::E& e) {
            auto g = [&] (item_supp_and_orderline_map::E& e2) {
                Stock x = e2.second.first;
                ol_map ol_m = ol_map::insert(e2.second.second, orderline[ol_number-1]);
                return make_pair(x, ol_m);
            };
            item_supp_and_orderline_map t1 = item_supp_and_orderline_map::update(e.second.second, make_pair(ol_i_id, ol_supply_w_id), g);
            return make_pair(e.second.first, t1);
        };
        sim.update(ol_suppkey, f);
    }

    {
        //update supp->part->lineitem
        auto f = [&] (supp_to_item_map_tmp::E& e) {
            auto g = [&] (item_supp_and_orderline_map_tmp::E& e2) {
                Stock x = e2.second.first;
                ol_map ol_m = ol_map::insert(e2.second.second, orderline[ol_number-1]);
                return make_pair(x, ol_m);
            };
            item_supp_and_orderline_map_tmp t1 = item_supp_and_orderline_map_tmp::update(e.second.second, ol_i_id, g);
            return make_pair(e.second.first, t1);
        };
        sim_tmp.update(ol_suppkey, f);
    }

    {
        //update part->supp->lineitem
        auto f = [&] (item_to_supp_map::E& e) {
            auto g = [&] (item_supp_and_orderline_map::E& e2) {
                ol_map ol_m = ol_map::insert(e2.second.second, orderline[ol_number-1]);
                return make_pair(e2.second.first, ol_m);
            };
            item_supp_and_orderline_map t1 = item_supp_and_orderline_map::update(e.second.second, make_pair(ol_i_id, ol_suppkey), g);
            return make_pair(e.second.first, t1);
        };
        ism.update(ol_i_id, f);
    }

    m.dm = dm;
    m.ism2 = ism;
    m.sim2 = sim;
    m.sim2_2 = sim_tmp;

    return t.stop();
}

double updateMap(string table_name, string pkeyColName, string pkeyVal, string colId, string value) {
    timer t; t.start();
    std::vector <std::string> pkeyColvec;
    std::vector <std::string> pkeyValvec;
    split(pkeyColName, std::string(";"), &pkeyColvec);
    split(pkeyVal, std::string(";"), &pkeyValvec);
    std::map <string, string> col_map;
    for (int i = 0; i < pkeyColvec.size() - 1; i++) {
        col_map[pkeyColvec[i]] = pkeyValvec[i];
    }

    if (table_name == "order") {
        // TRICKS: o_pkey = 100000*o_w_id + 10000* o_d_id + o_id
        // TRICKS: o_c_pkey = 100000*o_w_id + 10000* o_d_id + o_c_id;
        uint o_id = stoi(col_map["o_id"]);
        uint o_d_id = stoi(col_map["o_d_id"]);
        uint o_w_id = stoi(col_map["o_w_id"]);
        uint o_pkey = 100000 * o_w_id + 10000 * o_d_id + o_id;

        if (colId == "o_carrier_id") {
            // update om
            order_map om = m.om;
            cout << "Original: " << (*(m.om.find(o_pkey))).first.o_carrier_id;
            auto f = [&](order_map::E oe) {
                oe.second.first.o_carrier_id = stoi(value);
                return oe.second;
            };
            om.update(o_pkey, f);
            m.om = om;
            cout << ", Current: " << (*(m.om.find(o_pkey))).first.o_carrier_id << endl;
        } else {
            cout << colId << ": UNIMPLEMENTED" << endl;
        }
    } else if (table_name == "orderline") {
        // TRICKS: ol_pkey = 100000*ol_w_id + 10000* ol_d_id + ol_o_id
        uint ol_o_id = stoi(col_map["ol_o_id"]);
        uint ol_d_id = stoi(col_map["ol_d_id"]);
        uint ol_w_id = stoi(col_map["ol_w_id"]);

        if (colId == "ol_delivery_d") {

        } else {
            cout << colId << ": UNIMPLEMENTED" << endl;
        }
    } else if (table_name == "customer") {
        // TRICKS: c_pkey = 100000*c_w_id + 10000* c_d_id + c_id;
        uint c_id = stoi(col_map["c_id"]);
        uint c_d_id = stoi(col_map["c_d_id"]);
        uint c_w_id = stoi(col_map["c_w_id"]);
        uint c_pkey = 100000 * c_w_id + 10000 * c_d_id + c_id;

        if (colId == "c_balance") {
            // update cm
            customer_map cm = m.cm;
//            cout << "Original: " << (*(m.cm.find(c_pkey))).first.c_balance;
            auto f = [&](customer_map::E ce) {
                ce.second.first.c_balance = stoi(value);
                return ce.second;
            };
            cm.update(c_pkey, f);
            m.cm = cm;
//            cout << ", Current: " << (*(m.cm.find(c_pkey))).first.c_balance << endl;
        } else if (colId == "c_delivery_cnt") {
            // No need to update map
        } else {
            cout << colId << ": UNIMPLEMENTED" << endl;
        }
    } else if (table_name == "stock") {
        uint s_i_id = stoi(col_map["s_i_id"]);
        uint s_w_id = stoi(col_map["s_w_id"]);

        if (colId == "s_quantity") {
            // update ism
            item_supp_map ism = static_data.ism;
//            cout << "Original: " << (*(static_data.ism.find(make_pair(s_i_id, s_w_id)))).s_quantity;
            auto f = [&](item_supp_map::E se) {
                se.second.s_quantity = stoi(value);
                return se.second;
            };
            ism.update(make_pair(s_i_id, s_w_id), f);
            static_data.ism = ism;
//            cout << ", Current: " << (*(static_data.ism.find(make_pair(s_i_id, s_w_id)))).s_quantity << endl;
        } else if (colId == "s_order_cnt") {
            item_supp_map ism = static_data.ism;
//            cout << "Original: " << (*(static_data.ism.find(make_pair(s_i_id, s_w_id)))).s_order_cnt;
            auto f = [&](item_supp_map::E se) {
                se.second.s_order_cnt = stoi(value);
                return se.second;
            };
            ism.update(make_pair(s_i_id, s_w_id), f);
            static_data.ism = ism;
//            cout << ", Current: " << (*(static_data.ism.find(make_pair(s_i_id, s_w_id)))).s_order_cnt << endl;
        } else if (colId == "s_ytd" || colId == "s_remote_cnt") {
            // No need to update map
        } else {
            cout << colId << ": UNIMPLEMENTED" << endl;
        }
    }

    return t.stop();
}

double op(int op_type, int64_t txnid, string table_name, string pkey_col_name, string pkey_val, string col_name,
          string value) {
    double ret = 0;

    // update
    if (op_type == 0) {
        ret = updateMap(table_name, pkey_col_name, pkey_val, col_name, value);
        return ret;
    }

    // insert
    if (op_type == 1) {

        // useless cols, skip directly
        if ((col_name == "o_all_local") or (col_name == "ol_dist_info")) {
            return 0.0;
        }

        if (ready_insert.find(txnid) == ready_insert.end()) {
            vector <insert_form> v{insert_form{col_name, value}};
            ready_insert.insert(make_pair(txnid, v));
        } else {
            ready_insert[txnid].push_back(insert_form{col_name, value});
        }

        // Order has 8 cols
        if (table_name == "order" && ready_insert[txnid].size() == 7) {
            ret = insertOrder(txnid, ready_insert[txnid]);
            ready_insert.erase(txnid);
            return ret;
        }
        // Orderline has 10 cols
        if (table_name == "orderline" && ready_insert[txnid].size() == 9) {
            ret = insertOrderline(txnid, ready_insert[txnid]);
            ready_insert.erase(txnid);
            return ret;
        }

        return 0.0;
    }
}

pair<double, double> geo_mean(double *x, int round) {
    double res = 0.0, dev = 0.0;
    for (int j = 0; j < round; j++) {
        double tmp = log2(x[j] * 1000.0);
        res += tmp;
    }
    res = res / (round + 0.0);
    res = exp2(res);
    double m = 0.0;
    for (int j = 0; j < round; j++) {
        double tmp = log2(x[j] * 1000.0 / res);
        m += tmp * tmp;

    }
    dev = exp2(sqrt(m / (round + 0.0)));
    return make_pair(res, dev);
}

pair<double, double> arith_mean(double *x, int round) {
    double res = 0.0, dev = 0.0;
    for (int j = 0; j < round; j++) {
        double tmp = x[j] * 1000;
        res += tmp;
    }
    res = res / (round + 0.0);
    double m = 0.0;
    for (int j = 0; j < round; j++) {
        double tmp = (x[j] * 1000) / res;
        m += tmp * tmp;

    }
    dev = sqrt(m / (round + 0.0));
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
void output_res(double **tm, int round, int queries) {
    double tot = 0;
    double res[queries];
    for (int i = 0; i < queries; i++) {
        res[i] = 0;
        cout << "processing query " << i + 1 << endl;
        for (int j = 0; j < round; j++) {
            double tmp = log2(tm[i][j] * 1000.0);
            res[i] += tmp;
            cout << tm[i][j] << " ";
        }
        cout << endl;
        res[i] = res[i] / (round + 0.0);
        tot += res[i];
        res[i] = exp2(res[i]);
        double m = 0.0, dev = 0.0;
        for (int j = 0; j < round; j++) {
            double tmp = log2(tm[i][j] * 1000.0 / res[i]);
            m += tmp * tmp;
        }
        dev = exp2(sqrt(m / (round + 0.0)));
        cout << "geo mean of query " << i + 1 << " : " << res[i] << ", sdev: " << dev << endl << endl;
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
# include "Q21.h"
# include "Q22.h"

void test(string data_directory, bool verbose) {
    m = make_maps_test(data_directory, verbose);
//    int queries = 22, round = 6;
//    double** tm;
//    tm = new double*[queries];
//    for (int i = 0; i < queries; i++) {
//      tm[i] = new double[100];
//    }
//    for (int i = 0; i < round; i++) {
//        tm[0][i] = Q1time(m, verbose);
//        tm[1][i] = Q2time(m, verbose);
//        tm[2][i] = Q3time(m, verbose);
//        tm[3][i] = Q4time(m, verbose);
//        tm[4][i] = Q5time(m, verbose);
//        tm[5][i] = Q6time(m, verbose);
//        tm[6][i] = Q7time(m, verbose);
//        tm[7][i] = Q8time(m, verbose);
//        tm[8][i] = Q9time(m, verbose);
//        tm[9][i] = Q10time(m, verbose);
//        tm[10][i] = Q11time(m, verbose);
//        tm[11][i] = Q12time(m, verbose);
//        tm[12][i] = Q13time(m, verbose);
//        tm[13][i] = Q14time(m, verbose);
//        tm[14][i] = Q15time(m, verbose);
//        tm[15][i] = Q16time(m, verbose);
//        tm[16][i] = Q17time(m, verbose);
//        tm[17][i] = Q18time(m, verbose);
//        tm[18][i] = Q19time(m, verbose);
//        tm[19][i] = Q20time(m, verbose);
//        tm[20][i] = Q21time(m, verbose);
//        tm[21][i] = Q22time(m, verbose);
//    }
//
//    output_res(tm, round, queries);
//    updateMap("customer", "c_id;c_d_id;c_w_id;", "92;0;0;", "c_balance", "9999");
//    updateMap("stock", "s_i_id;s_w_id;", "9999999;0;", "s_order_cnt", "9999");
//    updateMap("order", "o_id;o_d_id;o_w_id;", "1623;0;0;", "o_carrier_id", "9999");
}

//int main(int argc, char **argv) {
//    commandLine P(argc, argv, "./test [-v] [-q] [-u] [-c] [-s size] [-t txns] [-d directory] [-y keep_versions] [-p]");
//    bool verbose = P.getOption("-v");
//    bool if_query = P.getOption("-q");
//    bool if_update = P.getOption("-u");
////  if_persistent = P.getOption("-p");
////  keep_versions = P.getOptionIntValue("-y", 1000000);
////  if_collect = P.getOption("-c");
//    string default_directory = "/ssd1/tpch/S10/";
//    int scale = P.getOptionIntValue("-s", 10);
//    int num_txns = P.getOptionIntValue("-t", 10000);
//    if (scale == 100) default_directory = "/ssd1/tpch/S100/";
//    if (scale == 1) default_directory = "/ssd1/tpch/S1/";
//
//    string data_directory = P.getOptionValue("-d", default_directory);
//
////  test_all(verbose, if_query, if_update,
////	   scale, num_txns, data_directory);
//
//    memory_stats();
//
//    test(data_directory, verbose);
//    return 0;
//}