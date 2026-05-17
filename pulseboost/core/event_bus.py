from __future__ import annotations

import asyncio
import inspect
from collections import defaultdict
from collections.abc import Awaitable, Callable
from typing import Any


EventHandler = Callable[[dict[str, Any]], Awaitable[None] | None]


class EventBus:
    def __init__(self) -> None:
        self._subscribers: dict[str, list[EventHandler]] = defaultdict(list)

    def subscribe(self, event_type: str, handler: EventHandler) -> None:
        self._subscribers[event_type].append(handler)

    async def publish(self, event_type: str, payload: dict[str, Any]) -> None:
        for handler in list(self._subscribers.get(event_type, [])):
            result = handler(payload)
            if inspect.isawaitable(result):
                await result

    async def publish_background(self, event_type: str, payload: dict[str, Any]) -> None:
        asyncio.create_task(self.publish(event_type, payload))
