#pragma once

#include <stdexcept>

#include "Globals.h"

class Event_manager
{
public:
    class Channel
    {
    private:
        std::mutex mutex;//locks everything

        std::unordered_map<Event_management::Event_type, std::vector<Event_management::Event_receiver_weak>> subscribers;

        std::queue<std::unique_ptr<Event_management::Event>> event_queue;
        std::unique_ptr<Event_management::Event> immediate_event;

        std::thread worker;

        std::condition_variable signal;
        bool running = true;

        std::shared_ptr<Channel> downstream = nullptr; // next channel to receive the event if this chanel didn't consume it

        /// <summary>
        ///     Event processing loop that handles immediate and queued events.
        /// </summary>
        /// <remarks>
        ///     Continuously waits for signals, processes immediate events instantly,
        ///     and processes queued events in order while managing thread safety.
        /// </remarks>
        /// <returns>None.</returns>
        void run() {
            while (running) {
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    signal.wait(lock, [&]
                    {
                        return  !running || immediate_event != nullptr || !event_queue.empty();
                    });

                    if (!running)
                        break;


                    if (immediate_event != nullptr )
                    {

                        if(immediate_event->timing == Event_management::Event_timing::Queued)
                        {
                            event_queue.push(std::move(immediate_event));
                            LOG_WARNING("The_event_manager - Queued event in immediate? is this intentional?\n"
								"moved it into que!");
                        }
                        else if (immediate_event->timing == Event_management::Event_timing::Immediate)
                        {
                            std::unique_ptr<Event_management::Event> local = std::move(immediate_event);
                            immediate_event = nullptr;
                            lock.unlock();
                            handle_event(std::move(local));
                            lock.lock();
                        }
                        else
                        {
                            LOG_FATAL("The_event_manager - Some unknown timing type: %d", (int)immediate_event->timing);
                            throw std::runtime_error("The_event_manager - Some unknown timing type :" + std::to_string((int)immediate_event->timing));
                        }

                        //Don't forget to move the event if needed before reset, or it will be destroyed!
                        //Probably unnecessary, just as a failsafe
                        if(immediate_event!=nullptr)
                            immediate_event.reset();
                    }
                    else
                    {
                        while (!event_queue.empty())
                        {
                            std::unique_ptr<Event_management::Event> event = std::move(event_queue.front());
                            event_queue.pop();

                            if(event->timing == Event_management::Event_timing::Immediate)
                            {
                                LOG_WARNING("The_event_manager - Immediate event in queue? is this intentional?\n"
									"turned it into Queued!");
                                event->timing = Event_management::Event_timing::Queued;
                            }

                            if (event->timing == Event_management::Event_timing::Queued)
                            {
                                lock.unlock();
                                handle_event(std::move(event));
                                lock.lock();
                            }
                            else
                            {
                                LOG_FATAL("The_event_manager - Some unknown timing type: %d", (int)event->timing);
                                throw std::runtime_error("The_event_manager - Some unknown timing type :" + std::to_string((int)event->timing));
                            }
                        }
                    }
                }
            }
        }

        /// <summary>
        ///     Handles and dispatches an event based on its scope (Targeted or Announcement).
        /// </summary>
        /// <param name="event">[in] Event to be processed and dispatched.</param>
        void handle_event(std::unique_ptr<Event_management::Event> event)
        {
            if (event->scope == Event_management::Event_scope::Targeted)
            {
                const Event_management::Event_receiver_shared shared_pointer = event->target_receiver.lock();
                if (shared_pointer != nullptr)
                {
                    //THE event call
                    (*shared_pointer)(*event);
                }
                else
                {
                    LOG_FATAL("The_event_manager - A targeted event has no target receiver!");
                    throw std::runtime_error("The_event_manager - A targeted event has no target receiver!");
                }
                if (event->is_alive)
                {
					LOG_WARNING("The_event_manager - targeted event is not consumed? is this intentional?\n"
						"consuming the event!");
                    event->is_alive = false;
                }
            }
            else if (event->scope == Event_management::Event_scope::Announcement)
            {
                const Event_management::Event_type type = event->type;

                std::vector<Event_management::Event_receiver_weak>& vec = subscribers[type];

                vec.erase(std::remove_if(vec.begin(), vec.end(),
                    [](const Event_management::Event_receiver_weak& w){ return w.expired(); }),
                    vec.end());

                for (const Event_management::Event_receiver_weak& receiver : vec)
                {
                    const Event_management::Event_receiver_shared shared_pointer = receiver.lock();
                    if(shared_pointer != nullptr)
                    {
                        (*shared_pointer)(*event);
                    }
                    else
                    {
                        LOG_FATAL("The_event_manager - I dont know how can this happen");
                        throw std::runtime_error("The_event_manager - I dont know how can this happen");
                    }
                }

                if(event->is_alive)
                {
                    if (downstream != nullptr)
                    {
                        downstream->throw_event(std::move(event));
                    }
                    else
                    {
                        event.reset();
                    }
                }
            }
            else
            {
                LOG_FATAL("The_event_manager - Some unknown scope type: %d", (int)event->scope);
                throw std::runtime_error("The_event_manager - Some unknown scope type :" + std::to_string((int)event->scope));
            }

        }

    public:

        Channel() {
            worker = std::thread(&Channel::run, this);
        }

        ~Channel() {
            {
                std::lock_guard<std::mutex> lock(mutex);
                running = false;
            }
            signal.notify_one();
            if (worker.joinable())
                worker.join();
        }

        /// <summary>
        ///     Subscribes a receiver to a specific event type.
        /// </summary>
        /// <param name="event_type">[in] Type of event to subscribe to.</param>
        /// <param name="receiver">[in] Event receiver to be notified.</param>
        void subscribe(const Event_management::Event_type event_type,const Event_management::Event_receiver_shared& receiver)
        {
            std::lock_guard<std::mutex> lock(mutex);
            subscribers[event_type].push_back(receiver);
        }

        /// <summary>
        ///     Pushes an event into the system for processing.
        ///     Immediate events are dispatched instantly, queued events are stored.
        /// </summary>
        /// <param name="event">[in] Event to be thrown into the event system.</param>
        void throw_event(std::unique_ptr<Event_management::Event> event)
        {
            std::unique_lock<std::mutex> lock(mutex);
            if(event->timing == Event_management::Event_timing::Immediate)
            {
                immediate_event = std::move(event);
                lock.unlock();//need to unlock so thread can lock it and work

                signal.notify_one();
            }
            else
            {
                event_queue.push(std::move(event));
            }
        }

        /// <summary>
        ///     Sets or changes the downstream channel for event forwarding.
        /// </summary>
        /// <param name="new_downstream">[in] New downstream channel (can be nullptr to remove).</param>
        void change_downstream(const std::shared_ptr<Channel>& new_downstream = nullptr)
        {
            this->downstream = new_downstream;
        }

        /// <summary>
        ///     Wakes up the event processing thread.
        /// </summary>
        void tick()
        {
            signal.notify_one();
        }

    };
private:
    std::unordered_map<std::string, std::shared_ptr<Channel>> channels;
    std::mutex channels_mutex;
public:

    /// <summary>
    ///     Creates a new event channel with the given name.
    /// </summary>
    /// <param name="name">[in] Name of the channel to create.</param>
    /// <returns>True if the channel was created, false if it already exists.</returns>
    bool create_channel(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(channels_mutex);
        if (channels.count(name))
        {
            LOG_ERROR("The_event_manager - channel '%s' already exists!", name.c_str());
            return false;
        }
        channels[name] = std::make_shared<Channel>();
        return true;
    }

    /// <summary>
    ///     Destroys an existing event channel by name.
    /// </summary>
    /// <param name="name">[in] Name of the channel to destroy.</param>
    void destroy_channel(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(channels_mutex);
        const auto it = channels.find(name);
        if (it == channels.end())
        {
            LOG_ERROR("The_event_manager - channel '%s' not exists!", name.c_str());
            return;
        }
        channels.erase(it);
    }

    /// <summary>
    ///     Returns the names of all registered event channels.
    /// </summary>
    /// <returns>Vector containing all channel names.</returns>
    std::vector<std::string> get_all_channel_names()
    {
        std::lock_guard<std::mutex> lock(channels_mutex);
        std::vector<std::string> names;
        names.reserve(channels.size());
        for (const auto& kv : channels)
            names.push_back(kv.first);
        return names;
    }

    /// <summary>
    ///     Retrieves an event channel by its name.
    /// </summary>
    /// <param name="name">[in] Name of the channel to retrieve.</param>
    /// <returns>Weak pointer to the channel, or an empty weak pointer if not found.</returns>
    std::weak_ptr<Channel> get_channel(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(channels_mutex);
        const auto it = channels.find(name);
        if (it == channels.end())
            return std::weak_ptr<Channel>();
        return std::weak_ptr<Channel>(it->second);
    }


    /// <summary>
    ///     Subscribes a receiver to a specific event type within a channel.
    /// </summary>
    /// <param name="channel_name">[in] Name of the channel.</param>
    /// <param name="event_type">[in] Type of event to subscribe to.</param>
    /// <param name="receiver">[in] Event receiver to be notified.</param>
    void subscribe(const std::string& channel_name, const Event_management::Event_type event_type, const Event_management::Event_receiver_shared& receiver)
    {
        std::lock_guard<std::mutex> lock(channels_mutex);
        const auto it = channels.find(channel_name);
        if (it == channels.end())
        {
            LOG_ERROR("The_event_manager - subscribe failed, channel '%s' not found!", channel_name.c_str());
            return;
        }
        it->second->subscribe(event_type, receiver);
    }


    //TODO: this will now probibly log lots of errors becouse layes will try to throw event in others without caring if they have the channel or not
    /// <summary>
    ///     Throws an event into a specific channel for processing.
    /// </summary>
    /// <param name="channel_name">[in] Name of the target channel.</param>
    /// <param name="event">[in] Event to be dispatched.</param>
    void throw_event(const std::string& channel_name, std::unique_ptr<Event_management::Event> event)
    {
        std::shared_ptr<Channel> channel_ptr;
        {
            std::lock_guard<std::mutex> lock(channels_mutex);
            const auto it = channels.find(channel_name);
            if (it == channels.end())
            {
                LOG_ERROR("The_event_manager - throw_event failed, channel '%s' not found!", channel_name.c_str());
                return;
            }
            channel_ptr = it->second; // copy shared_ptr while holding lock
        }

        // Call into the channel without holding channels_mutex
        channel_ptr->throw_event(std::move(event));
    }

    /// <summary>
    ///     Triggers a tick on the specified channel to wake its event processing thread.
    /// </summary>
    /// <param name="channel_name">[in] Name of the channel to tick.</param>
    void tick(const std::string& channel_name)
    {
        std::lock_guard<std::mutex> lock(channels_mutex);
        const auto it = channels.find(channel_name);
        if (it == channels.end())
        {
            LOG_ERROR("The_event_manager - tick failed, channel '%s' not found!", channel_name.c_str());
            return;
        }
        it->second->tick();
    }

    /// <summary>
    ///     Triggers a tick on all channels to wake their event processing threads.
    /// </summary>
    void tick_all()
    {
        std::lock_guard<std::mutex> lock(channels_mutex);
        for (auto& pair : channels)
        {
            pair.second->tick();
        }
    }

    // connect upstream -> downstream, pass nullptr as downstream to disconnect

    /// <summary>
    ///     Connects an upstream channel to a downstream channel for event forwarding.
    /// </summary>
    /// <param name="upstream_name">[in] Name of the upstream channel.</param>
    /// <param name="downstream">[in] Downstream channel (can be nullptr to disconnect).</param>
    /// <returns>True if connection succeeded, false otherwise.</returns>
    bool connect(const std::string& upstream_name, const std::shared_ptr<Channel>& downstream = nullptr)
    {
        std::lock_guard<std::mutex> lock(channels_mutex);

        const auto upstream_it = channels.find(upstream_name);
        if (upstream_it == channels.end())
        {
            LOG_ERROR("The_event_manager - connect failed, upstream '%s' not found!", upstream_name.c_str());
            return false;
        }

        upstream_it->second->change_downstream(downstream);
        return true;
    }
};