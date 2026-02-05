#pragma once

#include "crawler/bounded_queue.h"
#include <string>
#include <unordered_set>
#include <shared_mutex>
#include <optional>
#include <chrono>
#include <atomic>

class URLFrontier {
public:
    /**
     * URL Frontier
     * @param queue_capacity Maximum URLs in the queue
     */
    explicit URLFrontier(size_t queue_capacity = 10000)
        : queue_(queue_capacity) {}
    
    // Non-copyable but movable
    URLFrontier(const URLFrontier&) = delete;
    URLFrontier& operator=(const URLFrontier&) = delete;
    
    /**
     * Tries to add a URL to the frontier
     * Assumes URL is normalized and checked against visited set
     * @param url The URL to add
     * @param timeout How long to wait if queue is full
     * @return true if URL was added, false if duplicate/invalid/timeout/shutdown
     */
    bool try_add(const std::string& url, 
                 std::chrono::milliseconds timeout = std::chrono::milliseconds(100));
    
    /**
     * Tries to add a URL without blocking
     * @return true if URL was added
     */
    bool try_add_nowait(const std::string& url);
    
    /**
     * Adds multiple URLs (e.g., from parsing a page)
     * @param urls List of URLs to add
     * @return Number of URLs actually added
     */
    size_t add_batch(const std::vector<std::string>& urls);
    
    /**
     * Pops the next URL to crawl
     * @param timeout How long to wait if queue is empty
     * @return The URL, or nullopt if timeout/shutdown
     */
    std::optional<std::string> pop(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    
    /**
     * Checks if a URL has been visited
     * @param url The URL to check (will be normalized)
     */
    bool is_visited(const std::string& url) const;
    
    /**
     * Marks a URL as visited (called after successful fetch)
     * The URL should already be normalized
     */
    void mark_visited(const std::string& url);
    
    /**
     * Signal shutdown - unblocks all waiting threads
     */
    void shutdown();
    
    /**
     * Check if shutdown has been signaled
     */
    bool is_shutdown() const { return queue_.is_shutdown(); }
    
    /**
     * Get current queue size
     */
    size_t queue_size() const { return queue_.size(); }
    
    /**
     * Check if queue is empty
     */
    bool queue_empty() const { return queue_.empty(); }
    
    /**
     * Get number of visited URLs
     */
    size_t visited_count() const;
    
    /**
     * Get queue capacity
     */
    size_t capacity() const { return queue_.capacity(); }
    
    /**
     * Get statistics
     */
    struct Stats {
        size_t urls_added{0};
        size_t duplicates_skipped{0};
        size_t invalid_skipped{0};
    };
    
    Stats stats() const {
        return Stats{
            urls_added_.load(),
            duplicates_skipped_.load(),
            invalid_skipped_.load()
        };
    }

private:
    /**
     * Check if URL has been visited (internal, requires lock held)
     */
    bool is_visited_internal(const std::string& normalized_url) const;
    
    /**
     * Add to visited set (internal, requires lock held)
     * @return true if newly added, false if already present
     */
    bool add_to_visited(const std::string& normalized_url);
    
    BoundedQueue<std::string> queue_;
    std::unordered_set<std::string> visited_;
    mutable std::shared_mutex visited_mutex_;
    
    // Stats
    std::atomic<size_t> urls_added_{0};
    std::atomic<size_t> duplicates_skipped_{0};
    std::atomic<size_t> invalid_skipped_{0};
};
