/**
 ******************************************************************************
 * @file    spark_wiring_cloud.h
 * @author  Satish Nair, Zachary Crockett, Matthew McGowan
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#pragma once

#include "spark_wiring_string.h"
#include "events.h"
#include "system_cloud.h"
#include "system_sleep.h"
#include "spark_protocol_functions.h"
#include "spark_wiring_system.h"
#include "spark_wiring_watchdog.h"
#include "spark_wiring_async.h"
#include "spark_wiring_flags.h"
#include "interrupts_hal.h"
#include "system_mode.h"
#include <functional>

using particle::Flags;

typedef std::function<user_function_int_str_t> user_std_function_int_str_t;
typedef std::function<void (const char*, const char*)> wiring_event_handler_t;

#ifdef SPARK_NO_CLOUD
#define CLOUD_FN(x,y) (y)
#else
#define CLOUD_FN(x,y) (x)
#endif

#ifndef __XSTRING
#define	__STRING(x)	#x		/* stringify without expanding x */
#define	__XSTRING(x)	__STRING(x)	/* expand x, then stringify */
#endif

enum PublishFlag: uint8_t {
    PUBLIC = PUBLISH_EVENT_FLAG_PUBLIC,
    PRIVATE = PUBLISH_EVENT_FLAG_PRIVATE,
    NO_ACK = PUBLISH_EVENT_FLAG_NO_ACK,
    WITH_ACK = PUBLISH_EVENT_FLAG_WITH_ACK
};

PARTICLE_DEFINE_FLAG_OPERATORS(PublishFlag)

class CloudClass {


public:

    template <typename T, class ... Types>
    static inline bool variable(const T &name, const Types& ... args)
    {
        static_assert(!IsStringLiteral(name) || sizeof(name) <= USER_VAR_KEY_LENGTH + 1,
            "\n\nIn Particle.variable, name must be " __XSTRING(USER_VAR_KEY_LENGTH) " characters or less\n\n");

        return _variable(name, args...);
    }


    static inline bool _variable(const char* varKey, const bool& var)
    {
        return _variable(varKey, &var, BOOLEAN);
    }

    static inline bool _variable(const char* varKey, const int& var)
    {
        return _variable(varKey, &var, INT);
    }

#if PLATFORM_ID!=3
    // compiling with gcc this function duplicates the previous one.
    static inline bool _variable(const char* varKey, const int32_t& var)
    {
        return _variable(varKey, &var, INT);
    }
#endif

    static inline bool _variable(const char* varKey, const uint32_t& var)
    {
        return _variable(varKey, &var, INT);
    }

#if PLATFORM_ID!=3
    static bool _variable(const char* varKey, const float& var)
    __attribute__((error("Please change the variable from type `float` to `double` for use with Particle.variable().")));
#endif

    static inline bool _variable(const char* varKey, const double& var)
    {
        return _variable(varKey, &var, DOUBLE);
    }

    static inline bool _variable(const char* varKey, const String& var)
    {
        return _variable(varKey, &var, STRING);
    }

    static inline bool _variable(const char* varKey, const char* var)
    {
        return _variable(varKey, var, STRING);
    }

    template<std::size_t N>
    static inline bool _variable(const char* varKey, const char var[N])
    {
        return _variable(varKey, var, STRING);
    }

    template<std::size_t N>
    static inline bool _variable(const char* varKey, const unsigned char var[N])
    {
        return _variable(varKey, var, STRING);
    }

    static inline bool _variable(const char *varKey, const uint8_t* userVar, const CloudVariableTypeString& userVarType)
    {
        return _variable(varKey, (const char*)userVar, userVarType);
    }

    template<typename T> static inline bool _variable(const char *varKey, const typename T::varref userVar, const T& userVarType)
    {
        return CLOUD_FN(spark_variable(varKey, (const void*)userVar, T::value(), NULL), false);
    }

    static inline bool _variable(const char *varKey, const int32_t* userVar, const CloudVariableTypeInt& userVarType)
    {
        return CLOUD_FN(spark_variable(varKey, (const void*)userVar, CloudVariableTypeInt::value(), NULL), false);
    }

    static inline bool _variable(const char *varKey, const uint32_t* userVar, const CloudVariableTypeInt& userVarType)
    {
        return CLOUD_FN(spark_variable(varKey, (const void*)userVar, CloudVariableTypeInt::value(), NULL), false);
    }

    // Return clear errors for common misuses of Particle.variable()
    template<typename T, std::size_t N>
    static inline bool _variable(const char *varKey, const T (*userVar)[N], const CloudVariableTypeString& userVarType)
    {
        static_assert(sizeof(T)==0, "\n\nUse Particle.variable(\"name\", myVar, STRING); without & in front of myVar\n\n");
        return false;
    }

    template<typename T>
    static inline bool _variable(const T *varKey, const String *userVar, const CloudVariableTypeString& userVarType)
    {
        spark_variable_t extra;
        extra.size = sizeof(extra);
        extra.update = update_string_variable;
        return CLOUD_FN(spark_variable(varKey, userVar, CloudVariableTypeString::value(), &extra), false);
    }

    template<typename T>
    static inline bool _variable(const T *varKey, const String &userVar, const CloudVariableTypeString& userVarType)
    {
        static_assert(sizeof(T)==0, "\n\nIn Particle.variable(\"name\", myVar, STRING); myVar must be declared as char myVar[] not String myVar\n\n");
        return false;
    }

    template <typename T, class ... Types>
    static inline bool function(const T &name, Types ... args)
    {
#if PLATFORM_ID!=3
        static_assert(!IsStringLiteral(name) || sizeof(name) <= USER_FUNC_KEY_LENGTH + 1,
            "\n\nIn Particle.function, name must be " __XSTRING(USER_FUNC_KEY_LENGTH) " characters or less\n\n");
#endif
        return _function(name, args...);
    }

    static bool _function(const char *funcKey, user_function_int_str_t* func)
    {
        return CLOUD_FN(register_function(call_raw_user_function, (void*)func, funcKey), false);
    }

    static bool _function(const char *funcKey, user_std_function_int_str_t func, void* reserved=NULL)
    {
#ifdef SPARK_NO_CLOUD
        return false;
#else
        bool success = false;
        if (func) // if the call-wrapper has wrapped a callable object
        {
            auto wrapper = new user_std_function_int_str_t(func);
            if (wrapper) {
                success = register_function(call_std_user_function, wrapper, funcKey);
            }
        }
        return success;
#endif
    }

    template <typename T>
    static bool _function(const char *funcKey, int (T::*func)(String), T *instance) {
      using namespace std::placeholders;
      return _function(funcKey, std::bind(func, instance, _1));
    }

    inline particle::Future<bool> publish(const char *eventName, Flags<PublishFlag> flags1 = PUBLIC, Flags<PublishFlag> flags2 = Flags<PublishFlag>())
    {
        return publish(eventName, NULL, flags1, flags2);
    }

    inline particle::Future<bool> publish(const char *eventName, const char *eventData, Flags<PublishFlag> flags1 = PUBLIC, Flags<PublishFlag> flags2 = Flags<PublishFlag>())
    {
        return publish(eventName, eventData, 60, flags1, flags2);
    }

    inline particle::Future<bool> publish(const char *eventName, const char *eventData, int ttl, Flags<PublishFlag> flags1 = PUBLIC, Flags<PublishFlag> flags2 = Flags<PublishFlag>())
    {
        return publish_event(eventName, eventData, ttl, flags1 | flags2);
    }

    inline particle::Future<bool> publish(const char *eventName, const char *eventData, int ttl, PublishFlag eventType=PUBLIC)
    {
        return publish(eventName, eventData, ttl, PublishFlag::flag_t(eventType));
    }

    inline bool subscribe(const char *eventName, EventHandler handler, Spark_Subscription_Scope_TypeDef scope=ALL_DEVICES)
    {
        return CLOUD_FN(spark_subscribe(eventName, handler, NULL, scope, NULL, NULL), false);
    }

    inline bool subscribe(const char *eventName, EventHandler handler, const char *deviceID)
    {
        return CLOUD_FN(spark_subscribe(eventName, handler, NULL, MY_DEVICES, deviceID, NULL), false);
    }

    bool subscribe(const char *eventName, wiring_event_handler_t handler, Spark_Subscription_Scope_TypeDef scope=ALL_DEVICES)
    {
        return subscribe_wiring(eventName, handler, scope);
    }

    bool subscribe(const char *eventName, wiring_event_handler_t handler, const char *deviceID)
    {
        return subscribe_wiring(eventName, handler, MY_DEVICES, deviceID);
    }

    template <typename T>
    bool subscribe(const char *eventName, void (T::*handler)(const char *, const char *), T *instance, Spark_Subscription_Scope_TypeDef scope=ALL_DEVICES)
    {
        using namespace std::placeholders;
        return subscribe(eventName, std::bind(handler, instance, _1, _2), scope);
    }

    template <typename T>
    bool subscribe(const char *eventName, void (T::*handler)(const char *, const char *), T *instance, const char *deviceID)
    {
        using namespace std::placeholders;
        return subscribe(eventName, std::bind(handler, instance, _1, _2), deviceID);
    }

    void unsubscribe()
    {
        CLOUD_FN(spark_unsubscribe(NULL), (void)0);
    }

    bool syncTime(void)
    {
        return CLOUD_FN(spark_sync_time(NULL), false);
    }

    bool syncTimePending(void)
    {
        return connected() && CLOUD_FN(spark_sync_time_pending(nullptr), false);
    }

    bool syncTimeDone(void)
    {
        return !CLOUD_FN(spark_sync_time_pending(nullptr), false) || disconnected();
    }

    system_tick_t timeSyncedLast(void)
    {
        time_t dummy;
        return timeSyncedLast(dummy);
    }

    system_tick_t timeSyncedLast(time_t& tm)
    {
        tm = 0;
        return CLOUD_FN(spark_sync_time_last(&tm, nullptr), 0);
    }

    static void sleep(long seconds) __attribute__ ((deprecated("Please use System.sleep() instead.")))
    { SystemClass::sleep(seconds); }
    static void sleep(Spark_Sleep_TypeDef sleepMode, long seconds=0) __attribute__ ((deprecated("Please use System.sleep() instead.")))
    { SystemClass::sleep(sleepMode, seconds); }
    static void sleep(uint16_t wakeUpPin, InterruptMode edgeTriggerMode, long seconds=0) __attribute__ ((deprecated("Please use System.sleep() instead.")))
    { SystemClass::sleep(wakeUpPin, edgeTriggerMode, seconds); }

    static bool connected(void) { return spark_cloud_flag_connected(); }
    static bool disconnected(void) { return !connected(); }
    static void connect(void) {
        spark_cloud_flag_connect();
        if (system_thread_get_state(nullptr)==spark::feature::DISABLED &&
            SystemClass::mode() == SEMI_AUTOMATIC)
        {
            // Particle.connect() should be blocking in SEMI_AUTOMATIC mode when threading is disabled
            waitUntil(connected);
        }
    }
    static void disconnect(void) { spark_cloud_flag_disconnect(); }
    static void process(void) {
    		application_checkin();
    		spark_process();
    }
    static String deviceID(void) { return SystemClass::deviceID(); }

#if HAL_PLATFORM_CLOUD_UDP
    static void keepAlive(unsigned sec)
    {
        CLOUD_FN(spark_set_connection_property(particle::protocol::Connection::PING,
                                               sec * 1000, nullptr, nullptr),
                 (void)0);
    }
#endif

private:

    static bool register_function(cloud_function_t fn, void* data, const char* funcKey);
    static int call_raw_user_function(void* data, const char* param, void* reserved);
    static int call_std_user_function(void* data, const char* param, void* reserved);

    static void call_wiring_event_handler(const void* param, const char *event_name, const char *data);

    static particle::Future<bool> publish_event(const char *eventName, const char *eventData, int ttl, Flags<PublishFlag> flags);

    static ProtocolFacade* sp()
    {
        return spark_protocol_instance();
    }

    bool subscribe_wiring(const char *eventName, wiring_event_handler_t handler, Spark_Subscription_Scope_TypeDef scope, const char *deviceID = NULL)
    {
#ifdef SPARK_NO_CLOUD
        return false;
#else
        bool success = false;
        if (handler) // if the call-wrapper has wrapped a callable object
        {
            auto wrapper = new wiring_event_handler_t(handler);
            if (wrapper) {
                success = spark_subscribe(eventName, (EventHandler)call_wiring_event_handler, wrapper, scope, deviceID, NULL);
            }
        }
        return success;
#endif
    }

    static const void* update_string_variable(const char* name, Spark_Data_TypeDef type, const void* var, void* reserved)
    {
        const String* s = (const String*)var;
        return s->c_str();
    }

    // Test if the paramater a regular C "string" literal
    template <typename T>
    constexpr static bool IsStringLiteral(const T& param) {
      return std::is_array<T>::value && std::is_same<typename std::remove_extent<T>::type, char>::value;
    }
};


extern CloudClass Spark __attribute__((deprecated("Spark is now Particle.")));
extern CloudClass Particle;
