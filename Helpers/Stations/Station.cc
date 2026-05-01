#ifndef STATION_ACCESS
#define STATION_ACCESS
#endif

#include "../Dependencies/consts.hpp"

// Build a cheap (and fast) adjacency matrix for satellites. We only want 4 satellite neighbors
std::vector<std::vector<U16>> build_adjacency_matrix(Satellites* sats) {
    U16 total = sats->initialized_satellites;
    std::vector<std::vector<U16>> adj(total);

    for (U16 i = 0; i < total; i++) {
        std::vector<std::pair<float, U16>> visible_neighbors;
        Backend::Vector3 a{sats->positions.X[i], sats->positions.Y[i], sats->positions.Z[i]};

        for (U16 j = 0; j < total; j++) {
            if (i == j) continue;

            Backend::Vector3 b{sats->positions.X[j], sats->positions.Y[j], sats->positions.Z[j]};
            Backend::Vector3 c = b - a;
            float dist = mag_sq(c);

            float t = -(dot_func(a, c)) / dist;
            if (t >= 0.0f && t <= 1.0f) {
                Backend::Vector3 casted = a + (c * t);
                if (mag_sq(casted) < (R_EARTH * R_EARTH)) continue; // Blocked by Earth
            }

            visible_neighbors.push_back({dist, j});
        }

        int num_lasers = std::min(4, (int)visible_neighbors.size());
        std::partial_sort(visible_neighbors.begin(), 
                          visible_neighbors.begin() + num_lasers, 
                          visible_neighbors.end());

        for (int k = 0; k < num_lasers; k++) {
            adj[i].push_back(visible_neighbors[k].second);
        }
    }

    for (U16 i = 0; i < total; i++) {
        for (U16 neighbor : adj[i]) {
            auto it = std::find(adj[neighbor].begin(), adj[neighbor].end(), i);
            if (it == adj[neighbor].end()) {
                adj[neighbor].push_back(i);
            }
        }
    }

    return adj;
}

void calculate_dijkstra(Satellites* sats, r_t* inactive, const std::vector<std::vector<U16>>& adj_matrix){
    // Calculates a routing table for all satellites within sats
    unsigned int num_cores = std::thread::hardware_concurrency();
    if(num_cores == 0) num_cores = 16;

    if (num_cores > sats->initialized_satellites) num_cores = sats->initialized_satellites;

    int batch_size = sats->initialized_satellites / num_cores;

    std::vector<std::thread> threads;
    threads.reserve(num_cores);

    for(unsigned int L = 0; L < num_cores; L++){
        threads.emplace_back(std::thread([sats, inactive, num_cores, batch_size, L, &adj_matrix](){
            for(U16 cur_sat = L*batch_size; cur_sat < ((L == num_cores-1) ? sats->initialized_satellites : (L+1)*batch_size); cur_sat++){
                std::vector<float> distances(sats->initialized_satellites, std::numeric_limits<float>::max());
                std::vector<U16> previous_node(sats->initialized_satellites, 0);
                std::priority_queue<std::pair<float, U16>, std::vector<std::pair<float, U16>>, std::greater<std::pair<float, U16>>> min_pq;

                distances[cur_sat] = 0.0f;

                min_pq.emplace(0, cur_sat);

                while(!min_pq.empty()){
                    auto top = min_pq.top();
                    min_pq.pop();

                    float d = top.first;
                    U16 sat_id = top.second;

                    if(d > distances[sat_id]) continue;

                    Backend::Vector3 a{sats->positions.X[sat_id], sats->positions.Y[sat_id], sats->positions.Z[sat_id]};

                    for(U16 sat : adj_matrix[sat_id]){
                        Backend::Vector3 b{sats->positions.X[sat], sats->positions.Y[sat], sats->positions.Z[sat]};
                        Backend::Vector3 c = b - a;
                        
                        float edge_cost = std::sqrt(mag_sq(c));

                        if (distances[sat_id] + edge_cost < distances[sat]) {
                            distances[sat] = distances[sat_id] + edge_cost;
                            previous_node[sat] = sat_id;
                            min_pq.emplace(distances[sat], sat);
                        }
                    }
                }

                for(U16 sat = 0; sat < sats->initialized_satellites; sat++){
                    if(sat == cur_sat) continue;

                    if(distances[sat] == std::numeric_limits<float>::max()){
                        (*inactive)[cur_sat][sat] = std::numeric_limits<U16>::max();
                        continue;
                    }

                    U16 step = sat;
                    while(previous_node[step] != cur_sat) {
                        step = previous_node[step];
                    }

                    (*inactive)[cur_sat][sat] = step;
                }
            }
        }));
    }

    for(auto& t : threads){
       if(t.joinable()) t.join();
    }
}

int main(int argc, char* argv[]){
    U16 num_sats, num_users; 

    if(argc > 2){
        num_sats = std::stoi(argv[1]);
        num_users = std::stoi(argv[2]);
    } else{
        out("[Station] Enter number of satellites: ");
        std::cin >> num_sats;

        out("[Station] Enter number of users: ");
        std::cin >> num_users;
    }
    

    /*
    We want to emulate this communication scheme:
        User -> Satellite -> ... Satellite ... -> Ground Station/User

    Right now, our communication scheme is facing in this direction:
        User -> Ground Station -> ... Satellite ... -> Ground Station
    
    We need to get rid of the first Ground Station call. We can do this by:
        User:
            - Calculates optimal satellite to connect to (using Beam Planner)
            - Writes packet to the "global RF" (simulates writing to the satellite/sending it a signal) (shared memory between satellite driver and user driver)
        
        Satellite:
            - Reads from global RF buffer to pick up packets
            - Simulates latency, then routes them

    So where does the station come in play?
    The answer to that is whenever the satellites need it!
    
    The station will be responsible for generating the routing tables that the satellites will retrieve (from the shared memory).
    
    The station generates routing tables in advance, writes the current timestamped one to memory, and continues on with its life.
    Satellites retrieve their routing table entry, then go on with their lives.
    */

    int global_rf_space_fd = shm_open("/global_rf_space", O_CREAT | O_RDWR | O_TRUNC, 0666);
    // This shared memory will be for satellites communicating with the station (satellite requesting routing table, station providing routing table)
    int user_sat_rf_space_fd = shm_open("/user_sat_rf_space", O_CREAT | O_RDWR | O_TRUNC, 0666);

    unsigned long long len = sizeof(shared_mem_container);
    unsigned long long user_sat_len = sizeof(user_sat_mem);

    int truncate_result = ftruncate(global_rf_space_fd, len);
    int user_sat_truncate = ftruncate(user_sat_rf_space_fd, user_sat_len);

    if(truncate_result == -1){
        out("[Station] Failed to truncate fd [", global_rf_space_fd, "]! Err: ", errno);

        return 1;
    }

    if(user_sat_truncate == -1){
        out("[Station] Failed to truncate fd [", user_sat_rf_space_fd, "]! Err: ", errno);

        return 1;
    }

    // Ground station only ever truly communicates with the satellites

    // But our satellites will exist in shared memory (so that user can run beam planner on it)

    shared_mem_container* chunk1 = (shared_mem_container*)mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, global_rf_space_fd, 0);

    chunk1->initialized = true;

    out("[Station] Launching workers...");

    system(("cmd.exe /c start wsl ./satellite_worker " + std::to_string(num_sats)).c_str());
    system(("cmd.exe /c start wsl ./user_worker " + std::to_string(num_users)).c_str());

    out("[Station] Workers launched!");

    out("[Station] Launching routing calculation thread...");

    std::thread routing_calc = std::thread([chunk1](){
        out("[Station] Generating routing table...");

        bool ready = false;

        while(chunk1->initialized){
            if (chunk1->container.initialized_satellites == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            auto tab = chunk1->table.routing_table_key.load(std::memory_order_acquire);
            auto adj_m = build_adjacency_matrix(&chunk1->container);

            calculate_dijkstra(&chunk1->container, tab == 0 ? &chunk1->table.table_B : &chunk1->table.table_A, adj_m);

            chunk1->table.routing_table_key.store(tab == 0 ? 1 : 0, std::memory_order_release);

            if(!ready)[[unlikely]]{
                ready = true;
                out("[Station] Ready.");
            }

            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    });

    std::string opt;

    cli_prompt_hook = [&opt]() {
        return (opt == "exit" ? "" : ">> ");
    };

    while(std::cin >> opt){
        if(opt == "exit"){
            chunk1->initialized = false;

            if(routing_calc.joinable()) routing_calc.join();
            // unmap
            munmap((void*)chunk1, len);
            // close the fd
            close(global_rf_space_fd);
            // unlink
            shm_unlink("/station_satellite_communication");

            out("[Station] Shared memory cleaned up! Graceful exit.");

            break;
        } else if(opt == "request"){
            U16 sat_id;
            std::cin >> sat_id;

            out("[Station] Requesting satellite ", sat_id, " information...");

            float X = chunk1->container.positions.X[sat_id], Y = chunk1->container.positions.Y[sat_id], Z = chunk1->container.positions.Z[sat_id];
            
            out("[Station] Satellite [", sat_id, "]: <", X, ", ", Y, ", ", Z, "> [", std::sqrt(X*X + Y*Y + Z*Z), "]");
        }
    }
    
    return 0;
};