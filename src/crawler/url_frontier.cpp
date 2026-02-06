#include "include/crawler/url_frontier.h"

bool URLFrontier::try_add(const std::string& url, std::chrono::milliseconds timeout){
    // Need to add check for validity and normality first, will do later in utils

    // Check visited set with read lock first
    {
        std::shared_lock<std::shared_mutex> read_lock(visited_mutex_);
        if (visited_.count(url) > 0) {
            duplicates_skipped_++;
            return false;
        }
    } // shared_lock goes out of scope here, destructor called, unlocked

    // Not visited
    {
        std::unique_lock<std::shared_mutex> write_lock(visited_mutex_);
        auto [it, inserted] = visited_.insert(url);
        if (!inserted){
            duplicates_skipped_++;
            return false;
        }
    }

    // Successfully marked as visited, add to Bounded Queue
    if (queue_.push(url, timeout)){
        urls_added_++;
        return true;
    }
    
    // If unsuccessful, return false
    return false;

    
}