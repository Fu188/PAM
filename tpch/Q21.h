using Q21kip = pair<dkey_t, int>;

sequence <Q21kip> Q21(maps m, bool verbose, dkey_t q_nation) {
    supp_to_item_map spm = m.sim2;
    sequence <pair<uint, int>> res(SUPPLIER_NUM + 1);
    auto o_map_f = [&](supp_to_item_map::E &e, size_t i) {
        if (e.second.first.su_nationkey != q_nation) {
            res[i] = make_pair(e.second.first.su_suppkey, 0);
            return;
        }
        item_supp_and_orderline_map &med_map = e.second.second;
        auto m_map_f = [&](item_supp_and_orderline_map::E &e) -> int {
            ol_map &inner_map = e.second.second;
            auto i_map_f = [&](ol_map::E &l1) -> int {
                uint orderkey = l1.ol_pkey;
                uint suppkey = l1.ol_suppkey;
                maybe <order_map::V> maybex = m.om.find(orderkey);
                if (!maybex) return 0;
                Order o = (*(m.om.find(l1.ol_pkey))).first;
                if (!Date::less(o.o_entry_d, l1.ol_delivery_d)) return 0;
                order_map::V x = (*maybex);
                ol_map &y = x.second;
                auto cond1 = [&](ol_map::E &l2) {
                    if ((l2.ol_pkey == orderkey) &&
                          Date::less(l1.ol_delivery_d, l2.ol_delivery_d))
                        return true;
                    else return false;
                };
                if (!(ol_map::if_exist(y, cond1)))
                    return 1;
                else return 0;
            };
            int v = ol_map::map_reduce(inner_map, i_map_f, Add<int>());
            return v;
        };
        int v = item_supp_and_orderline_map::map_reduce(med_map, m_map_f, Add<int>(), 1);
        res[i] = Q21kip(e.second.first.su_suppkey, v);
    };
    supp_to_item_map::map_index(spm, o_map_f);

    auto less = [](Q21kip a, Q21kip b) { return a.second > b.second; };
    sequence <Q21kip> res2 = pbbs::sample_sort(res, less);

    return res2;
}

double Q21time(maps m, bool verbose) {
    timer t;
    t.start();
    dkey_t q_nation = 55;

    sequence <Q21kip> res2 = Q21(m, verbose, q_nation);
    double ret_tm = t.stop();

    cout << "Q21: " << ret_tm << endl;
    if (verbose) {
        for (int i=0; i<10; i++) {
            if (res2.size() <= i) continue;
            Q21kip r = res2[i];
            cout << "Q21:" << endl
                 << static_data.all_supp[r.first].su_name() << " " << r.second << endl;
        }
    }
    return ret_tm;
}
