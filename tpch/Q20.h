using Q20rtype = tuple<char *, char *>;

sequence <Q20rtype> Q20(maps m, bool verbose, const Date start, uint q_nation, const char *color) {
    supp_to_item_map_tmp &sim = m.sim2_2;
    auto supp_filter = [&](supp_to_item_map_tmp::E &se) {
        Supplier &s = se.second.first;
        if (s.su_nationkey != q_nation) return false;
        item_supp_and_orderline_map_tmp &inner_map = se.second.second;
        auto exists = [&](item_supp_and_orderline_map_tmp::E &e) {
            Stock &ps = e.second.first;
            uint itemkey = ps.s_i_id;
            Item &p = static_data.all_item[itemkey];
            ol_map &lm = e.second.second;
            auto map_orderline = [&](ol_map::E &l) -> int {
                if (Date::less_or_equal(l.ol_delivery_d, start)) return 0;
                return l.ol_quantity;
            };
            int th = ol_map::map_reduce(lm, map_orderline, Add<int>());
            if (strstr(p.i_data(), color) != p.i_data()) {
                return false;
            }
            if (((ps.s_quantity * 2) > th)) {
                return true;
            } else return false;
        };
        return item_supp_and_orderline_map_tmp::if_exist(inner_map, exists);
    };
    supp_to_item_map_tmp spm_nation = supp_to_item_map_tmp::filter(sim, supp_filter);


    sequence <Q20rtype> res(spm_nation.size());
    auto map_supp = [&](supp_to_item_map_tmp::E &e, size_t i) {
        Supplier &s = e.second.first;
        res[i] = Q20rtype(s.su_name(), s.su_address());
    };
    supp_to_item_map_tmp::map_index(spm_nation, map_supp);

    return res;
}

double Q20time(maps m, bool verbose) {
    timer t;
    t.start();

    char starts[] = "2010-05-23";
    const char color[] = "co";
    uint q_nation = 55;
    const Date start = Date(starts);

    sequence <Q20rtype> res = Q20(m, verbose, start, q_nation, color);

    double ret_tm = t.stop();
    cout << "Q20 : " << ret_tm << endl;
    if (verbose) {
        for (int i = 0; i < 10; i++) {
            if (res.size() <= i) continue;
            Q20rtype r = res[i];
            cout << "Q20:" << endl
                 << get<0>(r) << ", "
                 << get<1>(r) << endl;
        }
    }
    return ret_tm;
}
