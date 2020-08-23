#include "Grid.hpp"

#include <sstream>
#include <regex>
#include <string>
#include <cassert>
#include <cmath>
#include <iterator>
#include <array>
#include <algorithm>
#include <queue>
#include <iterator>
#include <stdexcept>






namespace nb {


Grid::Grid(int const width, int const height, std::string_view s)
    : m_width{width}
    , m_height{height}
    , m_total_black{width * height}
    , m_cells{}
    , m_regions{}
    , m_output{}
    , m_eng{1729} {



    if(width < 1) {
        throw std::runtime_error("width should be greater than 1: Grid::Grid()");
    }
    if(height < 1) {
        throw std::runtime_error("height should be greater than 1: Grid::Grid()");
    }

    m_cells.resize(width, std::vector<std::pair<State, std::shared_ptr<Region>>>
                   (height, std::make_pair(State::UNKNOWN, std::shared_ptr<Region>())));

    //Parse the string.
    std::vector<int> parser;
    std::regex const rx{R"((\d+)|( )|(\n)|[^\d \n])"};

    for(std::cregex_iterator i{s.begin(), s.end(), rx}, end; i != end; ++i) {

        auto const& m = *i;
        if(m[1].matched) {
            parser.push_back(std::stoi(m[1]));

        } else if(m[2].matched) {
            parser.push_back(0);

        } else if(m[3].matched) {
            //do nothing.

        } else {
            throw std::runtime_error("Grid::Grid(): Grid initialization contains invalid string.");
        }
    }

    if(parser.size() != static_cast<size_t>(width * height)) {
        throw std::runtime_error("Grid::Grid(): grid must contain "
                                 "string must contain width * height spaces and numbers");
    }

    for(auto x = 0; x < width; ++x) {
        for(auto y = 0; y < height; ++y) {
            auto const n = parser[x + y * width];

            if(n > 0) {


            }
            if(valid(x, y - 1) && static_cast<int>(cell(x, y - 1)) > 0) {
                throw std::runtime_error("Grid::Grid() the vertical adjacent cells "
                                         "string cannot be numbered.");
            }

            cell(x, y) = static_cast<State>(n);
            add_region(x, y);

            //Get the total number of black cells in the grid.
            m_total_black -= n;
        }

        print("I'm okay to go!");
    }
}

Grid::SitRep Grid::solve(bool const verbose, bool const guessing) {

    cache_map_t cache;

    if(known() == m_width * m_height) {
        if(detect_contradiction(verbose, cache)) {
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
            || detect_contradiction(verbose, cache)
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

    auto count_result = 0;
    for(auto x = 0; x < m_width; ++x) {
        for(auto y = 0; y < m_height; ++y) {

            if(cell(x, y) != State::UNKNOWN) {
                ++count_result;
            }
        }
    }
    return count_result;

}

void Grid::write(std::ostream& os, steady_clock_tp_t const start, steady_clock_tp_t const finish) const {

    os <<
       R"( <!DOCTYPE html>
       <html lang = "en">
       <head>
       <meta http-equiv="Content-type" content="text/html;charset=utf-8/">
       <style>
       body {
            font-family: Verdana, sans-serif;
            line-height: 1.4;
       }
       table {
            border: solid 3px #000000;
            border-collapse: collapse;
       }
       td {
            border: solid 1px #000000;
            text-align: center;
            width: 20px;
            height: 20px;
       }
       td.unknown{background-color: #C0C0C0;}
       td.white.new{background-color: #FFFF00;}
       td.white.old{}
       td.black.new{background-color: #008080;}
       td.black.old{background-color: #808080;}
       td.number{}
       td.failed{border:solid 3px #000000;}
       </style>
       <title>Nurikabe</title>
       </head>
       <body>

       )";

       steady_clock_tp_t old_ctr = start;

       for(auto const& [ s, v, updated, ctr, failed_guesses, failed_coords ] : m_output) {

            os << s << " (" << format_time(old_ctr, ctr) << ")\n";

            if(failed_guesses == 1) {
                os << "</br> 1 guess failed.\n";

            }else if(failed_guesses > 0){
                os << "</br>" << failed_guesses << "guesses failed.\n";
            }
            old_ctr = ctr;
            os << "<table>\n";
            for(int y = 0; y < m_height; ++y){
                os << "<tr>";

                for(int x = 0; x < m_width; ++x){
                        os << "<td class =\"";
                        os << (updated.find(std::make_pair(x, y)) != updated.end() ? "new " : "old ");
                        if(failed_coords.find(std::make_pair(x, y)) != failed_coords.end()) {
                            os << "failed ";
                        }

                        switch(v[x][y]) {
                        case State::UNKNOWN:
                            os << "unknown\"> ";
                            break;
                        case State::WHITE:
                            os << "white\"> ";
                            break;
                        case State::BLACK:
                            os << "black\"> ";
                        default:
                            os << "number\"> " << static_cast<int>(v[x][y]);
                            break;

                        }
                        os << "</td>\n";
                }
                os << "</tr>\n";
            }
            os << "</table><br/>\n";
       }
       os << "Total: " << format_time(start, finish) << "\n";
       os <<
            "</body>\n"
            "</html>\n";
}


//Region member function definition.
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



bool Grid::Region::is_white() const {
    return m_state == State::WHITE;
}

bool Grid::Region::is_black() const {
    return m_state == State::BLACK;
}

bool Grid::Region::is_numbered() const {
    return static_cast<int>(m_state) > 0;
}

int Grid::Region::number() const {
    assert(is_numbered());
    return static_cast<int>(m_state);
}

Grid::set_pair_t::const_iterator Grid::Region::begin() const {
    return m_coords.begin();
}

Grid::set_pair_t::const_iterator Grid::Region::end() const {
    return m_coords.end();
}

int Grid::Region::size() const {
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


int Grid::Region::unk_size() const {
    return static_cast<int>(m_unknowns.size());
}

void Grid::Region::unk_erase(int const x, int const y) {
    m_unknowns.erase(std::make_pair(x, y));
}
//End of region member function definition.


bool Grid::analyze_complete_islands(bool const verbose) {

    set_pair_t mark_as_black;
    set_pair_t mark_as_white;

    for(auto const& r : m_regions) {
        auto const& region = *r;

        //Check the region is numbered if it is
        //check its size if equal to the number required
        //the its complete and take the cells surround it, mark it as black.
        if(region.is_numbered() && region.size() == region.number()) {
            mark_as_black.insert(region.unk_begin(), region.unk_end());
        }
    }
    return process(verbose, mark_as_black, mark_as_white, "Complete island found!");

}

bool Grid::analyze_single_liberty(bool verbose) {
    set_pair_t mark_as_black;
    set_pair_t mark_as_white;

    for(auto const& region : m_regions) {

        auto const& r = *region;
        const bool is_partial = (r.is_black() && r.size() < m_total_black)
                                || r.is_white() || (r.is_numbered() && r.size() < r.number());

        if(is_partial && r.unk_size() == 1) {

            if(r.is_black()) {
                mark_as_black.insert(*r.unk_begin());
            } else {
                mark_as_white.insert(*r.unk_begin());
            }
        }
    }
    return process(verbose, mark_as_black, mark_as_white, "Expand partial region with only one liberty.");
}

bool Grid::analyze_dual_liberties(bool verbose) {

    set_pair_t mark_as_black;
    set_pair_t mark_as_white;

    for(auto const& region : m_regions) {

        auto const& r = *region;
        if(r.is_numbered() && r.size() == r.number() - 1 && r.unk_size() == 2) {
            auto const x1 = r.unk_begin()->first;
            auto const y1 = r.unk_begin()->second;

            auto const x2 = std::next(r.unk_begin())->first;
            auto const y2 = std::next(r.unk_begin())->second;

            if(std::abs(y1 - y2) == 1 && std::abs(x1 - x2) == 1) {

                std::pair<int, int> p;

                if(r.contains(x2, y1)) {
                    p = std::make_pair(x1, y2);

                } else {
                    p = std::make_pair(x2, y1);
                }

                if(cell(p.first, p.second) == State::UNKNOWN) {
                    mark_as_black.insert(p);
                }
            }
        }

        return process(verbose, mark_as_black, mark_as_white, "N - 1 with exactly diagonal two liberties found!");

    }
}

bool Grid::analyze_unreachable_cells(bool verbose) {

    set_pair_t mark_as_black;
    set_pair_t mark_as_white;

    for(auto x = 0; x < m_width; ++x) {
        for(auto y = 0; y < m_height; ++y) {

            if(unreachable(x, y)) {
                mark_as_black.insert(std::make_pair(x, y));
            }
        }
    }

    return process(verbose, mark_as_black, mark_as_white, "unreachable cells blackened.");

}

bool Grid::analyze_potential_pools(bool verbose) {
    set_pair_t mark_as_black;
    set_pair_t mark_as_white;

    for(auto x = 0; x < m_width - 1; ++x) {
        for(auto y = 0; y < m_height - 1; ++y) {

            struct XYState {
                int x{0};
                int y{0};
                State state;
            };

            std::array<XYState, 4> a{{
                    {x, y, cell(x, y)},
                    {x + 1, y, cell(x + 1, y)},
                    {x, y + 1, cell(x, y + 1)},
                    {x + 1, y + 1, cell(x + 1, y + 1)}
                }};

            static_assert(State::UNKNOWN < State::BLACK, "Assume State::UNKNOWN is less than State::BLACK.");

            std::sort(a.begin(), a.end(), [](auto const& lhs, auto const& rhs) {
                return lhs.state < rhs.state;
            });

            if(a[0].state == State::UNKNOWN
                    && a[1].state == State::BLACK
                    && a[2].state == State::BLACK
                    && a[3].state == State::BLACK) {

                mark_as_black.insert(std::make_pair(a[0].x, a[0].y));
            }
            if(a[0].state == State::UNKNOWN
                    && a[1].state == State::UNKNOWN
                    && a[2].state == State::BLACK
                    && a[3].state == State::BLACK) {

                for(int i = 0; i < 2; ++i) {
                    set_pair_t imagine_as_black;
                    imagine_as_black.insert(std::pair(a[0].x, a[0].y));
                    if(unreachable(a[1].x, a[1].y, imagine_as_black)) {
                        mark_as_white.insert(std::make_pair(a[0].x, a[0].y));
                    }

                    std::swap(a[0], a[1]);
                }

            }
        }
    }
    return process(verbose, mark_as_black, mark_as_white, "Prevent the potential pools.");
}

bool Grid::analyze_confinement(bool verbose, cache_map_t& cache) {
    set_pair_t mark_as_black;
    set_pair_t mark_as_white;

    for(int x = 0; x < m_width; ++x) {
        for(int y = 0; y < m_height; ++y) {
            if(cell(x, y) == State::UNKNOWN) {
                set_pair_t verboten;
                verboten.insert(std::make_pair(x, y));


                for(auto const& sp : m_regions) {
                    auto const& region = *sp;
                    if(confined(sp, cache, verboten)) {
                        if(r.black()) {
                            mark_as_black.insert(std::make_pair(x, y));

                        } else {
                            mark_as_white.insert(std::make_pair(x, y));
                        }
                    }
                }
            }
        }
    }

    for(auto const& sp : m_regions) {
        auto const& r = *sp;
        if(r.is_numbered() && r.size() < r.number()) {
            for(auto u = r.unk_begin(); u != r.unk_begin(); ++u) {
                set_pair_t verboten;
                verboten.insert(*u);

                insert_valid_unknown_neighbors(verboten, u->first, u->second);

                for(auto const& sp1 : m_regions) {
                    if(sp1 != sp && sp2->is_numbered && confined(sp1, cache, verboten));
                    mark_as_black.insert(*u);
                }
            }
        }
    }

    return process(verbose, mark_as_black, mark_as_white, "Confinement analysis succeeded.");

}

bool Grid::analyze_hypotheticals(bool const verbose) {
    set_pair_t mark_as_black;
    set_pair_t mark_as_white;

    auto const guessing_v = guessing_order;
    int failed_guesses = 0;
    set_pair_t failed_coords;

    for(auto const& [ x, y ] : guessing_v) {
        for(int i = 0; i < 2; ++i) {
            State const color = (i == 0) ? State::BLACK : State::WHITE;
            auto& mark_as_diff = (i == 0) ? mark_as_white : mark_as_black;
            auto& mark_as_same = (i == 0) ? mark_as_black : mark_as_white;

            Grid other(*this);
            other.mark(color, x, y);
            SitRep sr = SitRep::KEEP_GOING;

            while(sr == SitRep::KEEP_GOING) {
                sr = other.solve(false, false);
            }

            if(sr == SitRep::CONTRADICTION_FOUND) {
                mark_as_diff.insert(std::make_pair(x, y));
                return process(verbose, mark_as_black, mark_as_white, "Hypothetical contradiction found.",
                               failed_guesses, failed_coords);
            }

            if(sr == SitRep::SOLUTION_FOUND) {
                mark_as_same.insert(std::make_pair(x, y));
                return process(verbose, mark_as_black, mark_as_white, "Hypothetical solution found.",
                               failed_guesses, failed_coords);
            }

            ++failed_guesses;
            failed_coords.insert(std::make_pair(x, y));
        }
    }
    return false;
}

Grid::set_pair_t Grid::guessing_order() {

    std::vector<std::tuple<int, int, int>> x_y_manhattan;
    std::vector<std::pair<int, int>> white_cells;

    for(int x = 0; x < m_width; ++x) {
        for(int y = 0; y < m_height; ++y) {

            switch(cell(x, y)) {
            case State::UNKNOWN:
                x_y_manhattan.push_back(std::make_tuple(x, y, m_width + m_height));
                break;
            case State::WHITE:
                white_cells.push_back(x, y);
                break;
            default:
                break;

            }
        }
    }
    std::shuffle(x_y_manhattan.begin(), x_y_manhattan.end(), m_prng);

    for(auto const& [ x1, y1, manhattan ] : x_y_manhattan) {
        for(auto const& [ x2, y2 ] : white_cells) {
            manhattan = std::min(manhattan, std::abs(x1 - x2) + std::abs(y1 - y2));
        }
    }

    std::stable_sort(x_y_manhattan.begin(), x_y_manhattan.end(), [](auto const& lhs, auto const& rhs) {
        return std::get<2>(lhs) < std::get<2>(rhs);
    });

    std::vector<std::pair<int, int>> ret(x_y_manhattan.size());

    std::transform(x_y_manhattan.begin(), x_y_manhattan.end(),  ret.begin(), [](auto const& t) {
        return std::make_pair(std::get<0>(t), std::get<1>(t));
    });

    return ret;
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

void Grid::print(std::string_view s, set_pair_t const& updated,
                 int failed_guesses, set_pair_t const& failed_coords) {

    std::vector<std::vector<State>> new_grid(m_width, std::vector<State>(m_height));

    for(int x = 0; x < m_width; ++x) {
        for(int y = 0; y < m_height; ++y) {

            new_grid[x + y * m_width] = cell(x, y);
        }
    }
    m_output(std::make_tuple(s, new_grid, updated, std::chrono::steady_clock::now,
                             failed_guesses, failed_coords));

}

bool Grid::process(bool const verbose, set_pair_t const& mark_as_black,
                   set_pair_t const& mark_as_white, std::string_view s, int const failed_guesses,
                   set_pair_t const& failed_coords) {

    if(mark_as_black.empty() && mark_as_white.empty()) {
        return false;
    }
    for(auto const& [ x, y ] : mark_as_black) {

        mark(State::BLACK, x, y);
    }
    for(auto const& [ x, y ] : mark_as_white) {

        mark(State::WHITE, x, y);
    }

    if(verbose) {
        set_pair_t updated(mark_as_black);
        updated.insert(mark_as_white.begin(), mark_as_white.end());

        std::string t = s.data();

        if(m_sitRep == SitRep::CONTRADICTION) {
            t += "(Contradiction found! attempt to fuse two numbered regions or already known cells.)";
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

    auto r = std::make_shared<Region>(cell(x, y), unknowns, x, y);

    region(x, y) = r;
    m_regions.insert(r);

}

void Grid::mark(State const state, int x, int y) {
    assert((state == State::WHITE || state == State::BLACK) && "state must be either BLACK || WHITE.");

    if(cell(x, y) State::UNKNOWN) {
        m_sitRep = SitRep::CONTRADICTION;
        return;
    }

    cell(x, y) = state;
    for(auto const& sp : m_regions) {
        sp->unk_erase(x, y);
    }

    add_region(x, y);

    for_valid_neighbors(x, y, [x, y, this](auto a, auto b) {
        fuse_regions(region(x, y), region(a, b));
    });
}

//Shared_ptr are passed by value since are modified inside the function
//and we dont want that modification to be noticed by the caller.
void Grid::fuse_regions(std::shared_ptr<Region>r1, std::shared_ptr<Region>r2) {

    if(!r1 || !r2 || r1 == r2) {
        return;
    }
    if(r1->is_numbered() && r2->is_numbered()) {
        m_sitRep = SitRep::CONTRADICTION;
        return;
    }
    if(r1->is_black() != r2->is_black()) {
        return;
    }
    if(r1->size() < r2->size()) {
        std::swap(r2, r1);
    }
    if(r2->is_white()) {
        std::swap(r1, r2);
    }
    r1->insert(r2->begin(), r2->end());
    r1->unk_insert(r2->unk_begin(), r2->unk_end());

    for(auto&[x, y] : *r2) {
        region(x, y) = r1;
    }
    m_regions.erase(r2);
}

bool Grid::impossibly_big_white_region(int n) const {
    return std::none_of(m_regions.begin(), m_regions.end(), [n](auto const& sp) {

        return sp->is_numbered() && sp->size() + n + 1 <= sp->number;
    });
}

bool Grid::unreachable(int x_root, int y_root, set_pair_t discovered) {

    if(cell(x_root, y_root) != State::UNKNOWN) {
        return false;
    }

    std::queue<std::tuple<int, int, int>> q;

    q.push(std::make_tuple(x_root, y_root, 1));
    discovered.insert(std::make_pair(x_root, y_root));

    while(!q.empty()) {

        auto[x_curr, y_curr, n_curr] = q.front();
        q.pop();

        std::set<std::shared_ptr<Region>> white_regions;
        std::set<std::shared_ptr<Region>> numbered_regions;
        for_valid_neighbors(x_curr, y_curr, [&](auto const a, auto const b) {

            auto const& r = region(a, b);

            if(r && r->is_numbered()) {
                numbered_regions.insert(r);

            } else if(r && r->is_white()) {
                white_regions.insert(r);
            }
        });

        int n  = 0;

        for(auto const& sp : numbered_regions) {
            n += sp->size();
        }
        for(auto const& sp : white_regions) {
            n += sp->size();
        }

        if(numbered_regions.size() > 1) {
            continue;
        }
        if(numbered_regions.size() == 1) {

            auto const num = (*numbered_regions.begin())->number();
            if(n_curr + n <= num) {
                return false;

            } else {
                continue;
            }
        }
        if(!white_regions.empty()) {
            if(impossibly_big_white_region(n_curr + n)) {
                continue;

            } else {
                return false;
            }
        }

        for_valid_neighbors(x_curr, y_curr, [&](auto const a, auto const b) {

            if(cell(x_curr, y_curr) == State::UNKNOWN && discovered.insert(std::make_pair(a, b)).second) {
                q.push(std::make_tuple(a, b, n_curr + 1));
            }
        });


    }

    return true;

}

namespace {
enum struct Flags : unsigned char {

    NONE,
    OPEN,
    CLOSED,
    VERBOTEN,
};
}

bool Grid::confined(std::shared_ptr<Region>& r, cache_map_t& cache, set_pair_t const& verboten) {

    if(!verboten.empty()) {
        auto const i = cache.find(f);

        if(i == cache.end()) {
            return false;
        }
        auto const& consumed = i->second;

        if(std::none_of(verboten.begin(), verboten.end(), [](auto const& p) {
        return consumed.find(p) != consumed.end();
        })) {

            return false;
        }
    }

    std::vector<Flags> flags(m_width * m_height, Flags::NONE);
    for(auto i = r->unk_begin(); i != r->unk_end(); ++i) {
        auto const&[x, y] = *i;
        flags[x + y * m_width] = Flags::OPEN;
    }

    for(auto const&[x, y] : *r) {
        flags[x + y * m_width] = Flags::CLOSED;
    }

    for(auto const&[x, y] : verboten) {
        flags[x + y * m_width] = Flags::VERBOTEN;
    }
    size_t closed_size = r->size();

    while((r->is_black() && closed_size < m_total_black)
    || r->white || (r->is_numbered() && closed_size < r->number) {

    auto const iter = find(flags.begin(), flags.end(), Flags::OPEN);
        if(iter == end(flags)) {
            break;
        }

        *iter = Flags::NONE;

        size_t const index = std::distance(iter - flags.begin());
        const pair<int, int> p(index % m_width, index / m_width);
        auto const area = region(p.first, p.second);
        if(r->black()) {
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
                for_valid_neighbors(p.first, p.second, [&](auto const a, auto const b) {
                    auto const& other = region(a, b);
                    if(other && other->is_numbered() && other != r) {
                        rejected = true;
                    }
                });
                if(rejected) {
                    continue;
                }
            } else if(area->is_black()) {
                continue;

            } else if(area->is_white()) {

            } else {
                throw std::logic_error("logic error. Grid::confined. thought two numbered regions will be adjacent.");
            }

        }
        if(!area) {
            flags[p.first + p.second * m_width] = Flags::CLOSED;
            ++closed_size;

            for_valid_neighbors(p.first, p.second, [&](auto const a, auto const b) {
                Flags& f = flags[a + b * m_width];

                if(f == Flags::NONE) {
                    f = Flag::OPEN;
                }
            });
            if(verboten.empty()) {
                cache[r].insert(p);
            }
        } else {
            for(auto const&[ x, y ] : *area) {
                flags[x + y * m_width] = Flags::CLOSED;
            }
            closed_size += area->size();

            for(auto i = area->unk_begin(); i != unk_end(); ++i) {
                auto const&[ x, y ] = *i;
                Flags& f = flags[x + y * m_width];
                if(f == Flags::NONE) {
                    f = Flags::OPEN;
                }
            }
        }
    }
    return(r->is_black() && closed_size < m_total_black)
          || r->is_white()
          || (r->is_numbered() && closed_size < r->number());
}

bool Grid::detect_contradiction(bool verbose, cache_map_t& cache) {

    auto const Oops = [&](std::string_view s)->bool{
        if(verboten) {
            print(s);
        }
        SitRep::CONTRADICTION;
        return true;
    };

    for(int x = 0; x < m_width - 1; ++x) {
        for(int y = 0; y < m_height - 1; ++y) {

            if(cell(x, y) == State::BLACK
                    && cell(x + 1, y) == State::BLACK
                    && cell(x, y + 1) == State::BLACK
                    && cell(x + 1, y + 1) == State::BLACK) {

                return Oops("Contradiction Pools found!");
            }
        }
    }

    int black_cells = 0;
    int white_cells = 0;

    for(auto const r : m_regions) {
        auto const& region = *r;

        if((r.is_white() && impossibly_big_white_region(r.size()))
                || (r.is_numbered() && r.size() > r.number())) {

            return Oops("Contradiction found Gigantic region found!");
        }

        (r.black() ? black_cells : white_cells) = r.size();

        if(confined(r, cache)) {

            return Oops("Contradiction confined region!");
        }
    }
    if(black_cells > m_total_black) {
        return Oops("Contradiction too many black cells found!");
    }
    if(white_cells > m_width * m_height - m_total_black) {
        return Oops("Contradiction too many white cells found!");
    }

    return false;

}

std::string format_time(Grid::steady_clock_t const start, Grid::steady_clock_t const finish) {

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
