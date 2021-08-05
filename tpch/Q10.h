// c_custkey, c_name, revenue, c_acctbal, n_name, c_address, c_phone, c_comment
using Q10_relt = tuple<uint, char*, float, char*, char*, char*>;
using Q10_rtype = sequence<Q10_relt>;

Q10_rtype Q10(maps m, const char* start) {
  using kf_pair = pair<uint, float>;

  // get (custkey,revenue) for lineitems with return flag by order
  auto date_f = [&] (order_map::E& e) -> kf_pair {
    Order& o = e.second.first;
    auto ol_f = [&] (ol_map::E& ol) -> float {
        if (Date::greater_or_equal(o.o_entry_d, Date(start))
            && Date::less_or_equal(o.o_entry_d,ol.ol_delivery_d)) {
            return ol.ol_amount;
        }
        return 0.0;
    };
    float v = ol_map::map_reduce(e.second.second, ol_f, Add<float>());
    return make_pair(o.o_c_pkey*10000 + o.o_c_id, v);
  };
  sequence<kf_pair> elts = flatten<kf_pair>(m.oom, date_f);

  // sum revenue by custkey
  sequence<kf_pair> r = pbbs::collect_reduce_sparse(elts, pbbs::addm<float>());

  // sort by revenue
  auto less = [] (kf_pair a, kf_pair b) {return a.second > b.second;};
  sequence<kf_pair> res = pbbs::sample_sort(r, less);

  // generate results
  auto get_result = [&] (size_t i) {
    uint custkey = res[i].first%10000;
    float revenue = res[i].second;
    Customer c = (*(m.cm.find(res[i].first/10000))).first;
    char* n_name = static_data.all_nation[c.c_n_nationkey].name();
    return Q10_relt(custkey, c.c_last(), revenue, c.c_city(), c.c_phone(), n_name);
  };
    sequence<Q10_relt> rr(20, get_result);
    return rr;
}

double Q10time(maps m, bool verbose) {
  timer t;
  t.start();
  const char start[] = "2007-01-02";

  Q10_rtype result = Q10(m, start);
					    
  double ret_tm = t.stop();
  cout << "Q10 : " << ret_tm << endl;

  if (verbose) {
    Q10_relt r1 = result[0];
    cout << "Q10:" << endl;
    cout << get<0>(r1) << ", "
	 << get<1>(r1) << ", "
	 << get<2>(r1) << ", "
	 << get<3>(r1) << ", "
	 << get<4>(r1) << ", "
	 << get<5>(r1) << endl;
  }
  return ret_tm;
}

