"""
Standalone WebSocket bridge for Arduino devices
Run this ALONGSIDE FightTimer to provide plain WebSocket access

Usage:
    python arduino_websocket_bridge.py

This will:
- Connect to FightTimer's Socket.IO server (localhost:8765)
- Expose plain WebSocket server on port 8766
- Forward timer events from Socket.IO to WebSocket clients (Arduino)
"""

import asyncio
import json
import socketio
from aiohttp import web
import aiohttp

# Debug flag - set to False to disable forwarding debug messages for better performance
DEBUG_FORWARDING = False

# Socket.IO client to connect to FightTimer
sio = socketio.AsyncClient()

# Set of connected WebSocket clients (Arduino devices)
websocket_clients = set()

@sio.event
async def connect():
    """Called when connected to FightTimer's Socket.IO server"""
    print('Connected to FightTimer Socket.IO server')

@sio.event
async def disconnect():
    """Called when disconnected from FightTimer"""
    print('Disconnected from FightTimer Socket.IO server')

@sio.event
async def timer_update(data):
    """Receive timer updates from FightTimer and forward to Arduino clients"""
    if DEBUG_FORWARDING:
        print(f'Received timer update: {data}')
    
    # Forward to all connected WebSocket clients
    if websocket_clients:
        message = json.dumps(data)
        disconnected = set()
        
        for ws in websocket_clients:
            try:
                await ws.send_str(message)
                if DEBUG_FORWARDING:
                    print(f'Forwarded to Arduino: {message}')
            except Exception as e:
                print(f'Error sending to Arduino: {e}')  # Always show errors
                disconnected.add(ws)
        
        # Remove disconnected clients
        websocket_clients.difference_update(disconnected)

async def websocket_handler(request):
    """Handle WebSocket connections from Arduino devices"""
    ws = web.WebSocketResponse()
    await ws.prepare(request)
    
    print(f'Arduino WebSocket connected from {request.remote}')  # Always show connections
    websocket_clients.add(ws)
    
    try:
        async for msg in ws:
            if msg.type == aiohttp.WSMsgType.TEXT:
                # Arduino can send commands here if needed in the future
                if DEBUG_FORWARDING:
                    print(f'Received from Arduino: {msg.data}')
            elif msg.type == aiohttp.WSMsgType.ERROR:
                print(f'WebSocket error: {ws.exception()}')  # Always show errors
    finally:
        websocket_clients.discard(ws)
        print(f'Arduino WebSocket disconnected from {request.remote}')  # Always show disconnections
    
    return ws

async def start_bridge():
    """Start the bridge server"""
    # Connect to FightTimer's Socket.IO server
    print('Connecting to FightTimer at http://localhost:8765...')
    await sio.connect('http://localhost:8765')
    
    # Create aiohttp app for WebSocket server
    app = web.Application()
    app.router.add_get('/ws', websocket_handler)
    
    # Start WebSocket server
    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, '0.0.0.0', 8766)
    await site.start()
    
    print('Arduino WebSocket bridge running on port 8766')
    print('Arduino devices can connect to: ws://<your-ip>:8766/ws')
    print('Press Ctrl+C to stop')
    
    # Keep running
    try:
        await asyncio.Event().wait()
    except KeyboardInterrupt:
        print('\nShutting down...')
        await sio.disconnect()
        await runner.cleanup()

if __name__ == '__main__':
    asyncio.run(start_bridge())
