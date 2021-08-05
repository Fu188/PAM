struct Q3result {
  Q3result(uint ol_o_id, uint ol_w_id, uint ol_d_id, float r, Date entry_date)
    : ol_o_id(ol_o_id), ol_w_id(ol_w_id), ol_d_id(ol_d_id), revenue(r), o_entry_d(entry_date) {}
  Q3result() {}
  uint ol_o_id;
  uint ol_w_id;
  uint ol_d_id;
  float revenue;
  Date o_entry_d;
};
struct set_entry {
  using key_t = Q3result;
  static bool comp(const key_t& a, const key_t& b) {
    return (a.revenue > b.revenue);
  }
};
using rt = pam_seq<Q3result>;
using result_list = pam_set<set_entry>;

void Q3(maps m, bool verbose, char* seg, Date date) {
  
  customer_map& cm = m.cm;
  struct Union {
    using T = result_list;
    static T identity() {return result_list();}
    static T add(T a, T b) {
      return T::map_union(std::move(a), std::move(b));}
  };

  auto customer_f = [&] (customer_map::E& c) {
    char* c_state = c.second.first.c_state();
    if (strstr(c_state, seg) == c_state) {
      auto order_f = [&] (order_map::E& o) {
        Order& ord = o.second.first;
        if (Date::less(date, ord.o_entry_d)) {
          auto orderline_f = [&] (ol_map::E& ol) {
              return ol.ol_amount;
          };
          ol_map& lm = o.second.second;
          float revenue = ol_map::map_reduce(lm, orderline_f, Add<float>());
          if (revenue > 0.0)
            return result_list(Q3result(ord.o_id, ord.o_w_id, ord.o_d_id, revenue, ord.o_entry_d));
        }
        return result_list();
      };
      order_map& od = c.second.second;
      return order_map::map_reduce(od, order_f, Union());
    } else return result_list();
  };

  result_list r = customer_map::map_reduce(cm, customer_f, Union());

  Q3result x = *(r.select(0));
  if (verbose) {
	  cout << "Q3:" << endl;
	  cout << x.ol_o_id << ", "
	       << x.ol_w_id << ", "
	       << x.ol_d_id << ", "
	       << x.revenue << ", "
	       << x.o_entry_d.to_str() << endl;
  }
}

double Q3time(maps m, bool verbose) {
  timer t;
  t.start();
  char seg[] = "A";
  char sdate[] = "2007-01-02";
  Date date = Date(sdate);
  Q3(m, verbose, seg, date);
  double ret_tm = t.stop();
  cout << "Q3 : " << ret_tm << endl;

  return ret_tm;  
}

