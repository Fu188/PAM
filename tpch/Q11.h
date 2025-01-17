using ftype = double;
using Q11_elt = pair<uint, ftype>;
using Q11_rtype = pbbs::sequence<Q11_elt>;

Q11_rtype Q11(maps m, uint nation_id, double fraction) {
  timer t("Q11 detail", false);
  using kf_pair = Q11_elt;
  size_t max_item_key = (*(m.ism2.last())).first;
  pbbs::sequence<ftype> sums = pbbs::sequence<ftype>(max_item_key, (ftype) 0.0);
  t.next("init");
    
  auto supp_f = [&] (supp_to_item_map::E& se, size_t i) -> void {
    Supplier& s = se.second.first;
    if (s.su_nationkey == nation_id) {
      auto item_f = [&] (item_supp_and_orderline_map::E& pe, size_t j) -> void {
		Stock ps = pe.second.first;
		pbbs::write_add(&sums[ps.s_i_id], ps.s_order_cnt);
      };
      item_supp_and_orderline_map::map_index(se.second.second, item_f);
    }
  };
  supp_to_item_map::map_index(m.sim2, supp_f);
  t.next("map");

  auto rval = [&] (size_t i) {return kf_pair(i, sums[i]);};
  auto p = pbbs::delayed_seq<kf_pair>(max_item_key, rval);
  auto flag = [&] (size_t i) {return sums[i] > 0.0;};
  auto fl = pbbs::delayed_seq<bool>(max_item_key, flag);
  pbbs::sequence<kf_pair> r = pbbs::pack(p, fl);
  size_t n = r.size();
  t.next("filter");

  auto second_f = [&] (size_t i) -> ftype {return r[i].second;};
  double total = pbbs::reduce(pbbs::delayed_seq<ftype>(n, second_f),
			      pbbs::addm<ftype>());
  double cutoff = fraction*total;
  t.next("plus reduce");
      
  // only keep entries beyond a fraction of total
  // r is a sequence of <part,total_value> pairs
  pbbs::sequence<bool> big_enough(n, [&] (size_t i) {
      return r[i].second > cutoff;});
  pbbs::sequence<kf_pair> r2 = pbbs::pack(r, big_enough);
  t.next("pack");
 
  auto less = [] (kf_pair a, kf_pair b) -> bool {
    return a.second > b.second;};
  return pbbs::sample_sort(r2, less);
}


double Q11time(maps m, bool verbose) {
  timer t;
  t.start();
  // const char nation[] = "GERMANY";
  unsigned int nation_id = 55;
  double fraction = .005;
  Q11_rtype result = Q11(m, nation_id, fraction);
  double ret_tm = t.stop();
  cout << "Q11 : " << ret_tm << endl;
  
  if (verbose) {
      for (int i=0; i< 10; i++) {
          cout << "Q11:" << endl;
          if (result.size() <= i) cout << "Empty" << endl;
          else cout << result[i].first << ", " << result[i].second << endl;
      }
  }
  return ret_tm;
}

