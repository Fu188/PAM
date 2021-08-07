// nation, year, sum of profit
using Q9_elt = tuple<char *, int, double>;
using Q9_rtype = sequence<Q9_elt>;

Q9_rtype Q9(maps mx, const char *color) {
    int typelen = strlen(color);
    maps m = mx;
    using ps_map = item_to_supp_map;
    using s_map = item_supp_and_orderline_map;
    order_map &om = m.om;
    o_order_map &odm = m.oom;
    timer t;
    t.start();

    const uint num_nations = 123;
    const uint num_years = 50;
    size_t min_year = (*(odm.select(0))).first.year();
    size_t max_year = (*(odm.select(odm.size() - 1))).first.year();
    cout << min_year << ", " << max_year << endl;
    if (max_year - min_year + 1 > num_years) {
        cout << "year out of range in Q9" << endl;
        return Q9_rtype();
    }

    // write the order year out into a sequence indexed by order key
    size_t max_order_num = (*(om.select(om.size() - 1))).first;
    sequence <uint> ord_year(max_order_num + 1);
    auto order_f = [&](order_map::E &pe, size_t i) -> void {
        ord_year[pe.first] = (uint)(pe.second.first.o_entry_d.year() - min_year);
    };
    order_map::map_index(om, order_f);

    using nation_a = array<double, num_nations>;
    using nat_years = array<nation_a, num_years>;

    auto item_f = [&](nat_years &a, ps_map::E &pe) -> void {
        Item &p = pe.second.first;
        s_map &sm = pe.second.second;
        uint item_key = pe.first;
        char *cl = p.i_data();
        if (strcmp(cl + strlen(cl) - typelen, color) == 0) {
            auto supp_f = [&](s_map::E &se) -> void {
                ol_map &ol = se.second.second;
                auto ol_f = [&](ol_map::E &ol) -> void {
                    maybe<Stock> stock = static_data.ism.find(make_pair(ol.ol_i_id, ol.ol_supply_w_id ));
                    if (!stock) return;
                    Supplier &suppV = static_data.all_supp[(*stock).s_su_suppkey];
                    int natkey = suppV.su_nationkey;
                    double profit = ol.ol_amount;
                    int year = ord_year[ol.ol_pkey];
                    a[year][natkey] += profit;
                };
                ol_map::foreach_seq(ol, ol_f);
            };
            s_map::foreach_seq(sm, supp_f);
        }
    };

    item_to_supp_map &psm = m.ism2;
    using Add = Add_Nested_Array<nat_years>;
    nat_years a = ps_map::semi_map_reduce(psm, item_f, Add(), 4000);

    auto gen_results = [&](size_t i) -> Q9_elt {
        int nationkey = i / num_years;
        int year = (num_years - 1 - i % num_years);
        char *nat_name = static_data.all_nation[nationkey].name();
        double profit = a[year][nationkey];
        Q9_elt r(nat_name, year + min_year, profit);
        return r;
    };

    sequence <Q9_elt> rr(num_years * num_nations, gen_results);
    auto non_zero = [&](Q9_elt a) { return get<2>(a) != 0.0; };
    return pbbs::filter(rr, non_zero);
}

double Q9time(maps m, bool verbose) {
    timer t;
    t.start();
    const char color[] = "BB";

    Q9_rtype result = Q9(m, color);

    double ret_tm = t.stop();
    cout << "Q9 : " << ret_tm << endl;

    if (verbose) {
        for (int i = 0; i < 10; i++) {
            Q9_elt r = result[i];
            cout << "Q9:" << endl
                 << get<0>(r) << ", "
                 << get<1>(r) << ", "
                 << get<2>(r) << endl;
        }
    }

    return ret_tm;
}
