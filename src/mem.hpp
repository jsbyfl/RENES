
#pragma once

#include <map>
#include "type.hpp"
#include <vector>


namespace ReNes {
    
    const uint16_t MEMORY_WRITE_ONLY[] = {
        0x2000, 0x2001, 0x2003, 0x2005, 0x2006, 0x4014
    };

    struct Memory{

        Memory()
        {
            // 申请内存
            _data = (uint8_t*)malloc(0x10000);
            memset(_data, 0, 0x10000);
        }
        
        ~Memory()
        {
            if (_data)
                free(_data);
        }
        
        uint8_t* getIORegsAddr()
        {
            return &_data[0x2000];
        }
        
        uint8_t* masterData()
        {
            return _data;
        }
        
        // 读取数据
        uint8_t read8bitData(uint16_t addr, bool event=false, bool* cancel = 0)
        {
            auto data = *getRealAddr(addr, READ);
            
            // 处理2005,2006读取，每次读取之后重置bit7
//            switch (addr)
//            {
//                case 0x2005:
//                case 0x2006:
//                    ((bit8*)&_data[addr])->set(7, 0);
//            }
            
            // master访问不处理事件，或者是 0x4016 接口（0x4016属于特殊接口，映射控制器的值）
//            if (!master || addr == 0x4016)
            
            bool tmp;
            if (cancel == 0)
                cancel = &tmp;
            else
                *cancel = false;
            
            if (event)
            {
                // 检查地址监听
                if (SET_FIND(addr8bitReadingObserver, addr))
                {
                    addr8bitReadingObserver.at(addr)(addr, &data, cancel);
                }
            }

            return data;
        }
        
        // 16bit读取访问属于直接访问，没有事件处理
        uint16_t read16bitData(uint16_t addr) 
        {
            uint16_t data = *(uint16_t*)getRealAddr(addr, READ);
            
//            processReadingEvent(addr, &data);
            
            return data;
        }
        
        void write8bitData(uint16_t addr, uint8_t value)
        {
            *getRealAddr(addr, WRITE) = value;
            
//            if ((addr == 0x0100 + 0xfe || addr == 0x0100 + 0xff))
//            {
//                log("fuck\n");
//            }
            
            // 检查地址监听
            if (SET_FIND(addrWritingObserver, addr))
            {
                addrWritingObserver.at(addr)(addr, value);
            }
        }
        
        // 添加写入监听者
        void addWritingObserver(uint16_t addr, std::function<void(uint16_t, uint8_t)> callback)
        {
            addrWritingObserver[addr] = callback;
        }
        
        // 添加读取监听者
        void addReadingObserver(uint16_t addr, std::function<void(uint16_t, uint8_t*, bool*)> callback)
        {
            addr8bitReadingObserver[addr] = callback;
        }
        
        bool error = false;

    private:
        
        enum ACCESS{
            READ,
            WRITE,
            MASTER
        };
        
//        const std::vector<uint16_t> WRITE_ONLY = {
//        const static uint16_t WRITE_ONLY[] = {
//            0x2000, 0x2001, 0x2003, 0x2005, 0x2006, 0x4014
//        };

        // 得到实际内存地址
        uint8_t* getRealAddr(uint16_t addr, ACCESS access)
        {
            uint16_t fixedAddr = addr;
            
            // 处理I/O寄存器镜像
            if (addr >= 0x2008 && addr <= 0x3FFF)
            {
                fixedAddr = 0x2000 + (addr % 8);
            }
            // 2KB internal RAM 镜像
            else if (addr >= 0x0800 && addr <= 0x1FFF)
            {
                fixedAddr = addr % 0x0800;
            }
            
            
            
            // 2000 2001 只写
            // 2002 只读
            
            // 非法读取
//            if (access == READ && VECTOR_FIND(WRITE_ONLY, addr))
            if (access == READ && ARRAY_FIND(MEMORY_WRITE_ONLY, addr))
            {
                log("该内存只能写!\n");
                error = true;
                return (uint8_t*)0;
            }
            
            // 非法写入
            if (access == WRITE && (addr == 0x2002))
            {
                log("该内存只能读!\n");
                error = true;
                return (uint8_t*)0;
            }
            
            
            return _data + fixedAddr;
        }
        
        
        uint8_t* _data = 0;
        
        
        std::map<uint16_t, std::function<void(uint16_t, uint8_t*, bool*)>> addr8bitReadingObserver;
        std::map<uint16_t, std::function<void(uint16_t, uint8_t)>> addrWritingObserver;
    };
}
