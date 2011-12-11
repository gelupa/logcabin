/* Copyright (c) 2011 Stanford University
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR(S) DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#ifndef DLOGCLIENT_H
#define DLOGCLIENT_H

namespace DLogClient {

// forward declarations
class Cluster;
class ClusterImpl;

/**
 * The type of a log entry ID.
 * The first valid entry is 0.
 * Appends to the log are assigned monotonically increasing IDs, but some
 * numbers may be skipped.
 */
typedef uint64_t EntryId;

/**
 * A reserved log ID.
 */
static const EntryId NO_ID = ~0ULL;

/**
 * Encapsulates a blob of data in a single log entry.
 */
class Entry {
  private:
    /**
     * Constructor.
     * \param id
     *      The entry ID.
     * \param data
     *      Data that is owned by the caller.
     * \param length
     *      The number of bytes in data.
     */
    Entry(EntryId id, const void* data, uint32_t length);
  public:
    /**
     * Constructor.
     * In this constructor, the entry ID defaults to NO_ID.
     * \param data
     *      Data that is owned by the caller.
     * \param length
     *      The number of bytes in data.
     */
    Entry(const void* data, uint32_t length);
    ~Entry();
    /// Return the entry ID.
    EntryId getId() const;
    /// Return the binary blob of data.
    const void* getData() const;
    /// Return the number of bytes in data.
    uint32_t getLength() const;

  private:
    EntryId id;
    std::unique_ptr<const void> data;
    const uint32_t length;
    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;
    friend class Log;
};


/**
 * This exception is thrown when operating on a log that has been deleted.
 * It almost always indicates a bug in the application.
 */
class LogDisappearedException : public std::exception {
};

/**
 * A handle to a replicated log.
 * You can get an instance of Log through Cluster::openLog.
 */
class Log {
  private:
    Log(Cluster& cluster, const std::string& name, uint64_t logId);
  public:
    ~Log();

    /**
     * Append a new entry to the log.
     * \param data
     *      The blob to append.
     * \param invalidates
     *      A list of previous entries to be removed as part of this operation.
     * \param previousId
     *      Makes the operation conditional on this being the last ID in the
     *      log. Use NO_ID to unconditionally append.
     * \return
     *      The created entry ID, or NO_ID if the condition given by previousId
     *      failed.
     * \throw LogDisappearedException
     *      If this log no longer exists because someone deleted it.
     */
    EntryId append(Entry& data,
                   const std::vector<EntryId>& invalidates =
                        std::vector<EntryId>(),
                   EntryId previousId = NO_ID);

    /**
     * Invalidate entries in the log.
     * \param invalidates
     *      A list of previous entries to be removed as part of this operation.
     * \param previousId
     *      Makes the operation conditional on this being the last ID in the
     *      log. Use NO_ID to unconditionally append.
     * \return
     *      False if the condition given by previousId failed, otherwise true.
     * \throw LogDisappearedException
     *      If this log no longer exists because someone deleted it.
     */
    bool invalidate(const std::vector<EntryId>& invalidates,
                    EntryId previousId = NO_ID);

    /**
     * Read the entries starting at 'from' through head of the log.
     * \param from
     *      The entry at which to start reading.
     * \return
     *      The entries starting at and including 'from' through head of the
     *      log.
     * \throw LogDisappearedException
     *      If this log no longer exists because someone deleted it.
     */
    std::vector<Entry> read(EntryId from);

    /**
     * Return the ID for the head of the log.
     * \return
     *      The ID for the head of the log, or NO_ID if the log is empty.
     * \throw LogDisappearedException
     *      If this log no longer exists because someone deleted it.
     */
    EntryId getLastId();

  private:
    Cluster& cluster;
    const std::string name;
    const uint64_t logId;
    friend class Cluster;
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;
};

/**
 * Used to receive notifications of the DLog cluster being inaccessible.
 */
class ErrorCallback {
  public:
    virtual ~ErrorCallback();
    virtual void callBack(/* ... */);
};

/**
 * A handle to the DLog cluster.
 */
class Cluster {
  public:
    /**
     * Constructor.
     * \param hosts
     *      A comma-separated list of host:port, for example,
     *      192.168.1.10:2106,192.168.1.11:2106,192.168.1.12:2106
     */
    explicit Cluster(const std::string& hosts);
    ~Cluster();

    /**
     * Register a callback for the cluster being inaccessible.
     * \param callback
     *      The new callback.
     */
    void registerErrorCallback(std::unique_ptr<ErrorCallback> callback);

    /**
     * Open the log by the given name.
     * If no log by that name exists, one will be created.
     */
    Log openLog(const std::string& logName);

    /**
     * Open the log by the given name.
     * If no log by that name exists, one will be created.
     */
    void deleteLog(const std::string& logName);

    /**
     * Get a list of logs.
     * \return
     *      The name of each existing log.
     */
    std::vector<std::string> listLogs();

  private:
    std::unique_ptr<ClusterImpl> impl;
    Cluster(const Cluster&) = delete;
    Cluster& operator=(const Cluster&) = delete;
};

} // namespace

#endif /* DLOGCLIENT_H */