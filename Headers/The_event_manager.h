#pragma once

#include <stdexcept>

#include "Globals.h"

using namespace Event_management;

class Event_manager
{
private:
    class Channel
    {
    private:
        std::mutex mutex;//locks everything

        std::unordered_map<Event_type, std::vector<Event_receiver_weak>> subscribers;

        std::queue<std::unique_ptr<Event>> event_queue;
        std::unique_ptr<Event> immediate_event;

        std::thread worker;

        std::condition_variable signal;
        bool running = true;

        std::shared_ptr<Channel> downstream = nullptr; // next channel to receive the event if this chanel didn't consume it

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

                        if(immediate_event->timing == Event_timing::Queued)
                        {
                            event_queue.push(std::move(immediate_event));
                            LOG_WARNING("The_event_manager - Queued event in immediate? is this intentional?\n"
								"moved it into que!");
                        }
                        else if (immediate_event->timing == Event_timing::Immediate)
                        {
                            //TODO: if another immediate event comes during this, it throws an segmentation fault
                            //lock.unlock();
                            handle_event(std::move(immediate_event));
                            //lock.lock();
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
                            std::unique_ptr<Event> event = std::move(event_queue.front());
                            event_queue.pop();

                            if(event->timing == Event_timing::Immediate)
                            {
                                LOG_WARNING("The_event_manager - Immediate event in queue? is this intentional?\n"
									"turned it into Queued!");
                                event->timing = Event_timing::Queued;
                            }

                            if (event->timing == Event_timing::Queued)
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

        void handle_event(std::unique_ptr<Event> event)
        {
            if (event->scope == Event_scope::Targeted)
            {
                const Event_receiver_shared shared_pointer = event->target_receiver.lock();
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
            else if (event->scope == Event_scope::Announcement)
            {
                const Event_type type = event->type;

                std::vector<Event_receiver_weak>& vec = subscribers[type];

                vec.erase(std::remove_if(vec.begin(), vec.end(),
                    [](const Event_receiver_weak& w){ return w.expired(); }),
                    vec.end());

                for (const Event_receiver_weak& receiver : vec)
                {
                    const Event_receiver_shared shared_pointer = receiver.lock();
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
            worker.join();
        }

        // Subscribe an event_receiver to an event ID
        void subscribe(const Event_type event_type,const Event_receiver_shared& receiver)
        {
            std::lock_guard<std::mutex> lock(mutex);
            subscribers[event_type].push_back(receiver);
        }

        void throw_event(std::unique_ptr<Event> event)
        {
            std::unique_lock<std::mutex> lock(mutex);
            if(event->timing == Event_timing::Immediate)
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

        void change_downstream(const std::shared_ptr<Channel>& new_downstream = nullptr)
        {
            this->downstream = new_downstream;
        }

        void tick()
        {
            signal.notify_one();
        }

    };

    std::unordered_map<std::string, std::shared_ptr<Channel>> channels;
    std::mutex channels_mutex;

public:

    // Creates a channel if it doesn't exist, returns false if name already taken
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

    void subscribe(const std::string& channel_name, const Event_type event_type, const Event_receiver_shared& receiver)
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

    void throw_event(const std::string& channel_name, std::unique_ptr<Event> event)
    {
        std::lock_guard<std::mutex> lock(channels_mutex);
        const auto it = channels.find(channel_name);
        if (it == channels.end())
        {
            LOG_ERROR("The_event_manager - throw_event failed, channel '%s' not found!", channel_name.c_str());
            return;
        }
        it->second->throw_event(std::move(event));
    }

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

    void tick_all()
    {
        std::lock_guard<std::mutex> lock(channels_mutex);
        for (auto& pair : channels)
        {
            pair.second->tick();
        }
    }

    // connect upstream -> downstream, pass nullptr as downstream to disconnect
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