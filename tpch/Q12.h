// shipmode1 (high, low), shipmode2 (high low)
using Q12_relt = array<int, 2>;
using Q12_rtype = array<Q12_relt, 20>;

Q12_rtype Q12(maps m, char *end_date) {
    delivery_map delivery_range = delivery_map::upTo(m.dm, Date(end_date));

    using Add = Add_Nested_Array<Q12_rtype>;

    auto date_f = [&](delivery_map::E &order) -> Q12_rtype {
        auto ol_f = [&](Q12_rtype &a, ol_map::E &ol) {
            Order o = (*(m.om.find(ol.ol_pkey))).first;
            if (Date::less_or_equal(o.o_entry_d, ol.ol_delivery_d)){
                uint o_carrier_id = o.o_carrier_id;
                if (o_carrier_id == 1 || o_carrier_id == 2) {
                    a[o.o_ol_cnt][0] += 1;
                } else {
                    a[o.o_ol_cnt][1] += 1;
                }
            }
        };
        return ol_map::semi_map_reduce(order.second, ol_f, Add(), 2000);
    };
    return delivery_map::map_reduce(delivery_range, date_f, Add(), 1);
}

double Q12time(maps m, bool verbose) {
    timer t;
    t.start();
    char end_date[] = "2020-01-01";

    Q12_rtype result = Q12(m, end_date);

    double ret_tm = t.stop();
    if (query_out) cout << "Q12 : " << ret_tm << endl;

    if (verbose) {
        for (int i = 0; i<20; i++) {
            cout << "Q12:" << endl
                 << i << ", "
                 << result[i][0] << ", "
                 << result[i][1] << endl;
        }
    }
    return ret_tm;
}

