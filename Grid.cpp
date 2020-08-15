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





namespace nb_s {


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

void Grid::write(std::ostream& os, steady_clock_tp start, steady_clock_tp finish) const {

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


bool Grid::analyze_complete_islands(bool verbose) {

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
        const bool is_partial = r.is_black() && r.size() < m_total_black
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

}

bool Grid::process(bool verbose, set_pair_t const& mark_as_black, set_pair_t const& mark_as_white, std::string_view s) {

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

}

void Grid::mark(int x, int y) {

}

void Grid::fuse_regions(std::shared_ptr<Region>r1, std::shared_ptr<Region>r2) {

}

bool Grid::impossibly_big_white_region(int n) const {

}

bool Grid::unreachable(int x_root, int y_root, set_pair_t discovered) {

    if(cell(x_root, y_root) != State::UNKNOWN) {
        return false;
    }

    std::queue<std::tuple<int, int, int>> q;

    q.push(std::make_tuple(x_root, y_root, 1));
    discovered.insert(std::make_pair(x_root, y_root_));




}

bool Grid::confined(std::shared_ptr<Region>& r, cache_map_t& cache, set_pair_t const& verboten) {

}

bool Grid::detect_contradiction(bool verbose, cache_map_t& cache) {

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
