// s_acctbal, s_name, n_name, p_partkey, p_mfgr, s_address, s_phone, s_comment
using Q2_elt = tuple<uint, char *, char *, uint, char *, char *, char *, char *>;
using Q2_rtype = sequence<Q2_elt>;

bool equal(float a, float b) {
    if (abs(a - b) < 0.001) return true; else return false;
}

Q2_rtype Q2(maps m, unsigned int rid, int size, char *type) {

    int typelen = strlen(type);
    using i2sm = item_to_supp_map;
    using isom = item_supp_and_orderline_map;

    auto item_f = [&](i2sm::E &pe) -> maybe <i2sm::V> {
        Item &it = pe.second.first;
        isom &sim = pe.second.second;
        char *t = it.i_data();
        // if correct size and type
        if (strcmp(t + strlen(t) - typelen, type) == 0) {
            // for each supplier in correct region return ps cost
            auto supp_cost = [&](isom::E &se) -> float {
                //Supplier& s = se.second.first.second;
                uint suppkey = se.second.first.s_su_suppkey;
                Supplier &s = static_data.all_supp[suppkey];
                if (static_data.all_nation[s.su_nationkey].n_regionkey == rid) {
                    //return get_ps_cost(key_pair(p.partkey, s.suppkey));
                    return se.second.first.s_quantity;
                } else return 1e10;
            };
            // find the cheapest
            float min_cost = isom::map_reduce(sim, supp_cost, Min<float>());

            // for each supplier in correct region check if cheapest
            auto supp_f = [&](isom::E &se) -> bool {
                //Supplier& s = se.second.first.second;
                uint suppkey = se.second.first.s_su_suppkey;
                Supplier &s = static_data.all_supp[suppkey];
                return (static_data.all_nation[s.su_nationkey].n_regionkey == rid &&
                        se.second.first.s_quantity == min_cost);
            };

            // filter those in correct region that equal cheapest
            isom top = isom::filter(sim, supp_f);
            if (top.size() > 0) return maybe<i2sm::V>(make_pair(it, top));
            else return maybe<i2sm::V>();
        } else return maybe<i2sm::V>();
    };

    i2sm x = i2sm::map_filter(m.ism2, item_f);
    using elt = Q2_elt;
    // selected values
    auto ps_f = [&](i2sm::E pe, isom::E se) -> elt {
        Item it = pe.second.first;
        uint suppkey = se.second.first.s_su_suppkey;
        Supplier s = static_data.all_supp[suppkey];
        char *n_name = static_data.all_nation[s.su_nationkey].name();

        return elt(s.su_suppkey, s.su_name(), n_name, it.i_id, it.i_name(),
                   s.su_address(), s.su_phone(), s.su_comment());
    };
    sequence <elt> elts = flatten_pair<elt>(x, ps_f);

    // sort by output ordering: n_name, s_name, p_partkey
    auto less = [](elt a, elt b) {
        //return (get<0>(a) > get<0>(b));
        int cmp_n_name = strcmp(get<2>(a), get<2>(b));
        if (cmp_n_name < 0) return true;
        if (cmp_n_name > 0) return false;
        int cmp_s_name = strcmp(get<1>(a), get<1>(b));
        if (cmp_s_name < 0) return true;
        if (cmp_s_name > 0) return false;
        return get<3>(a) < get<3>(b);
    };
    return pbbs::sample_sort(elts, less);
}

double Q2time(maps m, bool verbose) {
    timer t;
    t.start();
    unsigned int rid = 3;
    int size = 15;
    char type[] = "b";
    Q2_rtype result = Q2(m, rid, size, type);
    //cout << result.size() << endl;

    double ret_tm = t.stop();
    cout << "Q2 : " << ret_tm << endl;

    if (verbose) {
        for (int i = 0; i < 10; i++) {
            Q2_elt r = result[i];
            cout << "Q2: " << endl
                 << get<0>(r) << ", " << get<1>(r) << ", "
                 << get<2>(r) << ", " << get<3>(r) << ", "
                 << get<4>(r) << ", " << get<5>(r) << ", "
                 << get<6>(r) << ", " << get<7>(r) << endl;
        }
    }
    return ret_tm;
}

