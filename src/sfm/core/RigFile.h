#pragma once

#include "sfm/core/Rig.h"

#include <istream>
#include <sstream>

namespace sfm {

inline RigTable readRigCalibration(std::istream& input, const std::string& filename,
                                   const std::vector<std::string>& names, std::vector<RigCalib>& calibration) {
    struct Row {
        std::string member, ref;
        Quat q;
        Vec3 t;
        uint32_t frames = 0;
        double spread = 0;
    };
    std::vector<std::string> order;
    std::map<std::string, std::vector<Row>> rows;
    for (std::string line; std::getline(input, line);) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string rig;
        Row r;
        if (!(ss >> rig >> r.member >> r.ref >> r.q[0] >> r.q[1] >> r.q[2] >> r.q[3] >> r.t.x >> r.t.y >> r.t.z))
            throw std::runtime_error(filename + ": cannot read '" + line + "'");
        ss >> r.frames >> r.spread;
        if (!rows.count(rig)) order.push_back(rig);
        rows[rig].push_back(std::move(r));
    }
    std::vector<RigDef> defs;
    for (const std::string& name : order) {
        RigDef d;
        d.name = name;
        for (const Row& r : rows[name]) d.members.push_back({r.member});
        defs.push_back(std::move(d));
    }
    RigTable rigs = buildRigTable(names, defs);
    calibration.assign(rigs.rigs.size(), RigCalib{});
    for (size_t r = 0; r < order.size(); r++) {
        const std::vector<Row>& rs = rows[order[r]];
        RigCalib& c = calibration[r];
        c.resize(rs.size());
        for (size_t k = 0; k < rs.size(); k++) {
            c.cam_from_rig[k] = {quaternionToRotation(rs[k].q), rs[k].t};
            c.established[k] = 1;
            c.support[k] = rs[k].frames;
            c.spread_deg[k] = rs[k].spread;
            if (rs[k].member == rs[k].ref) c.ref = (int)k;
        }
    }
    return rigs;
}

}  // namespace sfm
