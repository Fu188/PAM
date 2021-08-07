using Q4_rtype = array<int,6>;

Q4_rtype Q4(maps m, const char* start, const char* end) {
  o_order_map oom = m.oom;
  o_order_map order_range = o_order_map::range(oom, Date(start), Date(end));

  using T = Q4_rtype;
  using H = Add_Array<T>;

  auto date_f = [] (o_order_map::E& d) -> T {
    auto order_f = [] (T& a, order_map::E& o) -> void {
      auto orderline_f = [o] (OrderLine& e) -> int {
	    return Date::less_or_equal(o.second.first.o_entry_d, e.ol_delivery_d);
      };
      int n = ol_map::map_reduce(o.second.second, orderline_f, Add<int>());
      if (n > 0) a[o.second.first.o_ol_cnt]++;
    };
    return order_map::semi_map_reduce(d.second, order_f, H());
  };
  return o_order_map::map_reduce(order_range, date_f, H(), 1);
}

double Q4time(maps m, bool verbose) {
  timer t;
  t.start();
  const char start[] = "2007-01-02";
  const char end[] = "2012-01-02";

  Q4_rtype result = Q4(m, start, end);

  double ret_tm = t.stop();
  cout << "Q4 : " << ret_tm << endl;

  if (verbose) {
    for (int i=0; i<6;i++){
        int count = result[i];
        cout << 2007+i << ", " << count << endl;
    }
  }
  return ret_tm;
}


