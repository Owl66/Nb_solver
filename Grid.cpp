#include "Grid.hpp"
#include "Log.hpp"

#include <sstream>
#include <regex>
#include <string>
#include <cassert>
#include <array>
#include <algorithm>
#include <queue>
#include <cstddef>

namespace nb_s {

    Grid::Grid(int const width, int const height, std::string s)
        : m_width{width}
        , m_height{height}
        , m_total_black{width * height}
        , m_cells{}
        , m_sitRep{SitRep::KEEP_GOING}
        , m_regions{}
        , m_output{}
        , m_string{std::move(s)} 
        , m_eng{1729} {
        
        assert(width > 1 && height > 1 && "Width and height should be greater than 1. ");

        m_cells.resize(width, std::vector<std::pair<State, std::shared_ptr<Region>>>
                    (height, std::make_pair(State::UNKNOWN, std::shared_ptr<Region>())));

        //Parse the string.
        std::vector<int> vec_grid;
        std::regex const rx{R"((\d+)|( )|(\n)|[^\d \n])"};

        for(std::sregex_iterator i {m_string.begin(), m_string.end(), rx}, end; i != end; ++i) {

            auto const& m = *i;
            if(m[1].matched) {
                vec_grid.push_back(std::stoi(m[1]));

            } else if(m[2].matched) {
                vec_grid.push_back(0);

            } else if(m[3].matched) {
                //do nothing.

            } else {
                throw std::runtime_error("Grid::Grid(): Grid initialization contains invalid string.");
            }
        }

        assert(vec_grid.size() == static_cast<size_t>(width * height) && "Grid::Grid(): grid must contain  string must contain width * height spaces and numbers.");
        
        for(auto x = 0; x < width; ++x) {
            for(auto y = 0; y < height; ++y) {
                auto const n = vec_grid[x + y * width];

                if(n > 0) {

                    auto const cell_state =  static_cast<int>(cell(x, y - 1));
                    assert (valid(x, y - 1) && cell_state < 0 && "Grid::Grid() the vertical adjacent cells "
                                                    "string cannot be numbered.");
                }

                cell(x, y) = static_cast<State>(n);
                add_region(x, y);

                //Get the total number of black cells in the grid.
                m_total_black -= n;
            }
        }

        print("I'm okay to go!");
    }

    Grid::SitRep Grid::solve(bool const verbose, bool const guessing) {

        cache_map_t cache;

        if(known() == m_width * m_height) {
            if(detect_contradictions(verbose, cache)) {
                return SitRep::CONTRADICTION_FOUND;
            }

            if(verbose) {
                print("I'm done!");
            }

            return SitRep::SOLUTION_FOUND;
        }
        if(analyze_complete_islands(verbose)
            || analyze_single_liberty(verbose)
            || analyze_dual_liberties(verbose)
            || analyze_unreachable_cells(verbose)
            || analyze_potential_pools(verbose)
            || detect_contradictions(verbose, cache)
            || analyze_confinement(verbose, cache)
            || (guessing && analyze_hypotheticals(verbose))) {

                return m_sitRep;
            }
        if(verbose) {
            print("I'm stumped!");
        }

        return SitRep::CANNOT_PROCEED;
    }

    int Grid::known() const {
        auto ret = 0;
        
        for(auto x{0}; x < m_width; ++x){
            for(auto y{0}; y < m_height; ++y){
                if(cell(x, y) != State::UNKNOWN){
                    ++ret;
                }
            }
        }
        return ret;
    }

    void Grid::write(std::ostream& os, steady_clock_tp start, steady_clock_tp finish) const {
    }

    Grid::Region::Region(State const state, set_pair_t const& unknowns, int const x, int const y)
        : m_state{state}

        //Tracks the region.
        , m_coords{}

        //Tracks unknown cells surround the region.
        , m_unknowns{unknowns} {

        assert(state != State::UNKNOWN);

        //create the region.
        m_coords.insert(std::make_pair(x, y));
    }

    /*inline constexpr bool Grid::Region::is_white() const {
        return m_state == State::WHITE;
    }*/

    /*bool Grid::Region::is_black() const {
        return m_state == State::BLACK;
    }*/

    /*bool Grid::Region::is_numbered() const {
        return static_cast<int>(m_state) > 0;
    }*/

    int Grid::Region::its_number() const noexcept {
        assert(is_numbered());
        return static_cast<int>(m_state);
    }

    Grid::set_pair_t::const_iterator Grid::Region::begin() const {
        return m_coords.begin();
    }

    Grid::set_pair_t::const_iterator Grid::Region::end() const {
        return m_coords.end();
    }

    int Grid::Region::size() const noexcept {
        return static_cast<int>(m_coords.size());
    }

    bool Grid::Region::contains(int const x, int const y) const {
        return m_coords.find(std::make_pair(x, y)) != m_coords.end();
    }

    Grid::set_pair_t::const_iterator Grid::Region::unk_begin() const {
        return m_unknowns.begin();
    }

    Grid::set_pair_t::const_iterator Grid::Region::unk_end() const {
        return m_unknowns.end();
    }

    int Grid::Region::unk_size() const noexcept {
        return static_cast<int>(m_unknowns.size());
    }

    void Grid::Region::unk_erase(int const x, int const y) {
        m_unknowns.erase(std::make_pair(x, y));
    }

    bool Grid::analyze_complete_islands(bool verbose) {

        set_pair_t mark_as_black;
        set_pair_t mark_as_white;

        for(auto const& region : m_regions){
            auto const& r = *region;
            if(r.is_numbered() && r.size() == r.its_number()){
                mark_as_black.insert(r.unk_begin(), r.unk_end());
            }
        }
        return process(verbose, mark_as_black, mark_as_white, "Found the complete region.");
    }

    bool Grid::analyze_single_liberty(bool verbose) {
        set_pair_t mark_as_black;
        set_pair_t mark_as_white;

        for(auto const& region : m_regions) {
            auto const& r = *region;
            bool const partial = (r.is_black() && r.size() < m_total_black)
                                || r.is_white() 
                                || (r.is_numbered() && r.size() < r.its_number());

            if(partial && r.unk_size() == 1) {
                if(r.is_black()) {
                    mark_as_black.insert(*r.unk_begin());

                }else{
                    mark_as_white.insert(*r.unk_begin());
                }
            }

        }
        return process(verbose, mark_as_black, mark_as_white, "Expand partial region with single liberty. ");
    }

    bool Grid::analyze_dual_liberties(bool verbose) {
        set_pair_t mark_as_black;
        set_pair_t mark_as_white;

        for(auto const& region: m_regions){
            auto const& r = *region;
            if(r.is_numbered() && r.size() == r.its_number() - 1 && r.unk_size() == 2){
                auto const x1 = r.unk_begin()->first;
                auto const y1 = r.unk_begin()->second;

                auto const next  = std::next(r.unk_begin());
                auto const x2 = next->first;
                auto const y2 = next->second;

                auto p = std::pair<int, int>{};
                if(std::abs(x1 - x2) == 1 && std::abs(y1 - y2) == 1) {
                    if(r.contains(x2, y1)) {
                        p = std::make_pair(x1, y2);

                    }else {
                        p = std::make_pair(x2, y1);
                    }
                }
                if(cell(p.first, p.second) == State::UNKNOWN) {
                    mark_as_black.insert(p);
                }
            }
        }
        return process(verbose, mark_as_black, mark_as_white, " N - 1 islands whith two completely diagonal liberties");
    }

    bool Grid::analyze_unreachable_cells(bool verbose) {

        set_pair_t mark_as_black;
        set_pair_t mark_as_white;

        for(auto x{0}; x < m_width; ++x) {
            for(auto y{0}; y < m_height; ++y) {
                if(unreachable(x, y)) {
                    mark_as_black.insert(std::make_pair(x, y));
                }
            }
        }
        return process(verbose, mark_as_black, mark_as_white, "Unreachable cell blackened. ");

    }

    bool Grid::analyze_potential_pools(bool verbose) {
        set_pair_t mark_as_black;
        set_pair_t mark_as_white;

        for(auto x{0}; x < m_width - 1; ++x) {
            for(auto y{0}; y < m_height - 1; ++y) {

                struct XY {
                    int x;
                    int y;
                    State state;
                };
                std::array<XY, 4> quadrant { {
                   { x, y, cell(x, y) },
                   { x + 1, y, cell(x + 1, y) },
                   { x, y + 1, cell(x, y + 1) },
                   { x + 1, y, cell(x + 1, y) }
                } };
                static_assert(State::BLACK > State::UNKNOWN, " Black should be greater than state::unknown.");
                std::sort(begin(quadrant), end(quadrant), [](auto const lhs, auto const rhs){
                    return lhs.state < rhs.state;
                });
                if(quadrant[0].state == State::UNKNOWN
                && quadrant[1].state == State::BLACK
                && quadrant[2].state == State::BLACK
                && quadrant[3].state == State::BLACK) {

                    mark_as_white.insert(std::make_pair(quadrant[0].x, quadrant[0].y));
                
                }else if(quadrant[0].state == State::UNKNOWN
                    && quadrant[1].state == State::UNKNOWN
                    && quadrant[2].state == State::BLACK
                    && quadrant[3].state == State::BLACK) {
                    
                    for(auto i{0}; i < 2; ++i) {
                        set_pair_t imagine_black;
                        imagine_black.insert(std::make_pair(quadrant[0].x, quadrant[0].y));
                        if(unreachable(quadrant[1].x, quadrant[1].y, imagine_black)) {
                            mark_as_white.insert(std::make_pair(quadrant[0].x, quadrant[0].y));
                        }

                        std::swap(quadrant[0], quadrant[1]);
                    }
                }
            }
        }
        return process(verbose, mark_as_black, mark_as_white, " Analysis the potential pool. ");
    }

    bool Grid::analyze_confinement(bool verbose, cache_map_t& cache) {
        set_pair_t mark_as_black;
        set_pair_t mark_as_white;
        
        for(auto x{0}; x < m_width; ++x) {
            for(auto y{0}; y < m_height; ++y) {
                if(cell(x, y) == State::UNKNOWN) {
                    set_pair_t verboten;
                    verboten.insert(std::make_pair(x, y));

                    for(auto const& sp : m_regions) {
                        auto const& r = *sp;
                        if(confined(sp, cache, verboten)) {
                            if(r.is_black()) {
                                mark_as_black.insert(std::make_pair(x, y));

                            } else {
                                mark_as_white.insert(std::make_pair(x, y));

                            }
                        }
                    }
                }
            }
        }
        for(auto const& sp1 : m_regions) {
            auto const& r = *sp1;
            if(r.is_numbered() && r.size() < r.its_number()) {
                for(auto u{r.unk_begin()}; u != r.unk_end(); ++u) {
                    set_pair_t verboten;
                    verboten.insert(*u);

                    insert_valid_neighbors(verboten, u->first, u->second);

                    for(auto const& sp2 : m_regions) {
                        if(sp2 != sp1 && sp2->is_numbered() && confined(sp2, cache, verboten)) {
                            mark_as_black.insert(*u);
                        }
                    }
                    
                }
            } 
        }
        return process(verbose, mark_as_black, mark_as_white, "Confinment analysis succeeded.");
    }

    bool Grid::analyze_hypotheticals(bool verbose) {
    }

    Grid::set_pair_t Grid::guessing_order() {
    }

    bool Grid::valid(int x, int y) {
        return x >= 0 && x < m_width && y >= 0 && y < m_height;
    }

    Grid::State const& Grid::cell(int x, int y) const {
        return m_cells[x][y].first;
    }

    Grid::State& Grid::cell(int x, int y) {
        return m_cells[x][y].first;
    }

    std::shared_ptr<Grid::Region>& Grid::region(int x, int y) {
        return m_cells[x][y].second;
    }

    std::shared_ptr<Grid::Region> const& Grid::region(int x, int y) const {
        return m_cells[x][y].second;
    }

    void Grid::print(std::string const& s, set_pair_t const& updated, int failed_guesses, set_pair_t const& failed_coords) {

        std::vector<std::vector<State>> v(m_width, vector<State>(m_height));

        for(auto x = 0; x < m_width; ++x) {
            for(auto y = 0; y < m_height; ++y) {
                v[x][y] = cell(x, y);
            }
        }
        m_output.push_back(std::make_tuple(s, v, updated, steady_clock::now(), failed_guesses, failed_coords));
    }

    bool Grid::process(bool verbose, set_pair_t const& mark_as_black, set_pair_t const& mark_as_white, std::string const& s) {

        if(mark_as_black.empty() && mark_as_white.empty()) {
            return false;
        }
        for(auto const& [ x, y ] : mark_as_black){
            mark(State::BLACK, x, y);
        }
        for(auto const& [ x, y ] : mark_as_black){
            mark(State::WHITE, x, y);
        }
        if(verbose) {
            set_pair_t updated(mark_as_black);
            updated.insert(mark_as_white.begin(), mark_as_white.end());

            std::string t = s;
            if(m_sitRep == SitRep::CONTRADICTION_FOUND) {
                t += "Contradiction Found attempt to fuse two numbered region or mark marked cell";
                LOG("[WARNING]Contradiction mark known cell or attempt to fuse numbered regions");
            }
            print(t, updated, failed_guesses, failed_coords);
        }
        return true;
    }

    void Grid::insert_valid_neighbors(set_pair_t& s, int x, int y) const { 
        for_valid_neighbors(x, y, [&](auto const a, auto const b) {
            s.insert(std::make_pair(a, b));
        });
    }

    void Grid::insert_valid_unknown_neighbors(set_pair_t& s, int x, int y) const {
        for_valid_neighbors(x, y, [&](auto const a, auto const b) {
            if(cell(a, b) == State::UNKNOWN) {
                s.insert(std::make_pair(a, b));
            }
        });
    }

    void Grid::add_region(int x, int y) {
        set_pair_t unknowns;
        insert_valid_unknown_neighbors(unknowns, x, y);

        auto r = std::make_shared<Grid::Region>(cell(x, y), unknowns, x, y);

        region(x, y) = r;
        m_regions.insert(r);

    }

    void Grid::mark(State const state, int x, int y) {
        assert((state == State::BLACK || state == State::WHITE) && "Expect state not to be State::Unknown.");

        if(cell(x, y) != State::UNKNOWN) {
            m_sitRep = SitRep::CONTRADICTION_FOUND;
            LOG("Found contradiction");
            return;
        }
        cell(x, y) = state;
        for(auto const& sp : m_regions) {
            sp->unk_erase(x, y);
        }
        add_region(x, y); 

        for_valid_neighbors(x, y, [this, x, y](auto const a, auto const b) {
            fuse_regions((region(x, y)), region(a, b));
        });

    }

    void Grid::fuse_regions(std::shared_ptr<Region> r1, std::shared_ptr<Region> r2) {

        if(!r1 || !r2 || r1 == r2) {
            return;
        }

        if(r1->is_black() != r2->is_black()) {
            return;
        }

        if(r1->is_numbered() && r2->is_numbered()) {
            m_sitRep == SitRep::CONTRADICTION_FOUND;
            return;
        }
        if(r2->size() > r1->size()) {
            swap(r1, r2);
        }
        if(r2->is_numbered()) {
            swap(r1, r2);
        }
        r1->insert(r2->begin(), r2->end());
        r1->unk_insert(r2->unk_begin(), r2->unk_end());

        for(auto const& [ x, y ] : *r2) {
            region(x, y) = r1;
        }
        m_regions.erase(r2);

    }

    bool Grid::impossibly_big_white_region(int n) const { 
        return std::none_of(begin(m_regions), end(m_regions), [=](auto const sp) {
            sp->is_numbered() && sp->size() + n + 1 <= sp->its_number();
        });
    }

    bool Grid::unreachable(int x_root, int y_root, set_pair_t discovered) {

        if(cell(x_root, y_root) != State::UNKNOWN) {
            return false;
        }
        std::queue<std::tuple<int, int, int>> q;
        q.push(std::make_tuple(x_root, y_root, 1));
        
        while(!q.empty()) {

            auto [ x_curr, y_curr, n_curr ] = q.front();
            q.pop();

            std::set<std::shared_ptr<Region>> white_region;
            std::set<std::shared_ptr<Region>> numbered_region;

            for_valid_neighbors(x_curr, y_curr, [&](auto const a, auto const b){
                std::shared_ptr<Region> const& r = region(a, b);
                LOG("[] The neighbors are obtained in line 499.");
                if(r && r->is_white()){
                    white_region.insert(r);

                } else if (r && r->is_numbered()) {
                    numbered_region.insert(r);
                }
            });

            size_t size{0};

            for(auto const& sp : white_region) {
                size += sp->size();
            }
            for(auto const& sp : numbered_region) {
                size += sp->size();
            }
            if(numbered_region.size() > 1) {
                LOG("[WARNING] Area of two numbered region found!");
                continue;
            }
            if(numbered_region.size() == 1) {
                int const num = (*numbered_region.begin())->its_number();
                if(n_curr + size <= num) {
                    return false;

                } else {
                    continue;
                }
            } 
            if(!white_region.empty()) {
                if(impossibly_big_white_region(n_curr + size)) {
                    LOG("[WARNING] impossibly big white region found!");
                    continue;

                } else {
                    return false;
                }
            }
            for_valid_neighbors(x_curr, y_curr, [&](auto const a, auto const b){
                if(cell(a, b) == State::UNKNOWN && discovered.insert(std::make_pair(a, b)).second) {
                    q.push(std::make_tuple(a, b, n_curr + 1));
                }
            });

        }
        return true;
    }

    namespace {
        enum Flag : unsigned char{
            NONE,
            OPEN, 
            CLOSED, 
            VERBOTEN,
        };

    }//end of namespace.

    bool Grid::confined(std::shared_ptr<Region> const& r, cache_map_t& cache, set_pair_t const& verboten) {

        if(!verboten.empty()) {
            auto const i = cache.find(r);

            if(i == cache.end()) {
                return false;
            }

            auto const& consumed = i->second;
            if(std::none_of(verboten.begin(), verboten.end(), [&](auto const& a) {
                return consumed.find(p) != consumed.end();
            })) {

                return false;
            }
            std::vector<Flag> flags(m_width * m_height, NONE);

            for(auto i{r->unk_begin()}; i != r->unk_end(); ++i) {
                auto const& [ x, y ] = *i;
                flags[x + y * m_width] = OPEN;
            }

            for(auto const& [ x, y ] : *r) {
                flags[x + y * m_width] = CLOSED;
            }

            int closed_size = r->size();

            for(auto const& [ x, y ] : verboten) {
                flags[x + y * m_width] = VERBOTEN;
            }


        }

        while((r->is_black() && closed_size < m_total_black)
            || r->is_white()
            ||(r->is_numbered() && closed_size < r->is_numbered())) {

                auto const iter = find(flags.begin(), flags.end(), OPEN);
                if(iter == flags.end()) {
                    break;
                }
                *iter = NONE;
                size_t index = static_cast<size_t>(iter - flags.begin());

                const pair<int, int> p(index % m_width, index / m_width);
                const auto& area = region(p.first, p.second);
                if(r->is_black()) {
                    if(!area) {

                    } else if(area->is_black()) {

                    } else {
                        continue;
                    }
                } else if(r->is_white()) {
                    if(!area) {

                    } else if(area->is_black()) {
                        continue;
                    } else if(area->is_white()) {

                    } else {
                        return false;
                    }
                } else {
                    if(!area) {

                        bool rejected = false;

                        for_valid_neighbors(p.first, p.second, [&](auto const a, auto cont b) {
                            auto const& other = region(a, b);
                            if(othe && other->is_numbered() && other != r) {
                                rejected = true;
                            }
                        });
                        
                        if(rejected) {
                            continue;
                        }
                    } else if(area->is_black()) {
                        continue;
                    }else if(area->is_white()) {
                        
                    } else {
                        throw std::logic_error("Grid::confined()- i was confused and thought "
                        "two numbered region would be adjacent");
                    }
                }
            }
        }
        if(!area) {
            flags[p.first + p.second * m_width] = CLOSED;
            ++closed_size;

            for_valid_neighbors(p.first, p.second, [&](auto const a, auto const b) {
                Flag& flag = flags[a + b * m_width];
                if(f == NONE) {
                    f = OPEN;
                }
            });
            
            if(verboten.empty()) {
                cache[r].insert(p);
            }
        } else {
            for(auto const& [ x, y ] : *area) {
                flags[x + y * m_width] = CLOSED;
            }
            closed_size += area->size();
            for(auto i = area->unk_begin(); i != area->unk.end(); ++i) {
                auto const& [ x, y ] = *i;
                flag& f = flags[x + y * m_width];

                if(f == NONE) {
                    f = OPEN;
                }
            }
        }

        return(r->is_black() && closed_size < m_total_black)
            || r->is_white() 
            || (r->is_numbered() && closed_size < r->is_numbered());
    }

    bool Grid::detect_contradictions(bool verbose, cache_map_t& cache) {

        auto uh_oh = [&](std::string const& s)->bool {
            if(verbose) {
                print(s);
            }
            m_SitRep = SitRep::CONTRADICTION_FOUND;
            return true;
        }
        for(auto x = 0; x < m_width - 1; ++x) {
            for(auto y = 0; y < m_height - 1; ++y) {
                if(cell(x, y) == State::BLACK
                && cell(x + 1, y) == State::BLACK
                && cell(x, y + 1) == State::BLACK
                && cell(x + 1, y + 1) == State::BLACK) {

                    return uh_oh("Contradiction found! Pool detected.");
                    LOG("[WARNING]Contradiction pool detected.");
                

                }
            }
        }
        int black_cells = 0;
        int white_cells = 0;
        for(const auto& sp : m_regions) {
            const Region& r = *sp;

            if((r.is_white() && impossibly_big_white_region(r.size()))
               || (r.is_numbered() && r.size() > r.its_number())) {

                return  uh_oh("Contradiction! Gigantic region detected.");
                LOG("[WARNING]Gigantic region detected");
            }
            (r.is_black() ? black_cells : white_cells) += r.size();
            if(confined(sp, cache)) {
                return uh_oh("Contradiction! confined region found.");
                LOG("[WARNING]Confined region");

            }
            if(black_cells > m_total_black) {
                return uh_oh("Contradiction! Too many black cells.");
                LOG("[WARNING]Too many black cells");
            }
            if(white_cells > m_width * m_height - m_total_black) {
                return uh_oh("Contradiction! Too many white/numbered cells found.");
                LOG("[WARNING]Many numbered/white cells found");
            }
            return false;

        }
    }

    std::string format_time(Grid::steady_clock_tp const start, Grid::steady_clock_tp const finish) {

        std::ostringstream ostream;
        using namespace std::literals::chrono_literals;

        if(finish - start < 1s) {
            ostream << std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(finish - start).count()
                    << " milliseconds" ;

        } else if(finish - start < 1ms) {
            ostream << std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(finish - start).count()
                    << " microseconds";

        } else {
            ostream << std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count()
                    << " seconds";
        }
        return ostream.str();
    }
}// end of namespace nb_s
