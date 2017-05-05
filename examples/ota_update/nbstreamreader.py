# from http://eyalarubas.com/python-subproc-nonblock.html
# (heavily) modified by Jannik Beyerstedt

import threading
from Queue import Queue, Empty

class UnexpectedEndOfStream(BaseException):
    pass

class StoppableThread(threading.Thread):
    """Thread class with a stop() method. The thread itself has to check
    regularly for the stopped() condition."""

    def __init__(self, target=None, args=()):
        self.target = target
        self.args = args
        super(StoppableThread, self).__init__(target=self.target, args=self.args)
        self._stop = threading.Event()

    def stop(self):
        self._stop.set()

    def stopped(self):
        return self._stop.isSet()

class NonBlockingStreamReader:
    def __init__(self, stream):
        '''
        stream: the stream to read from.
                Usually a process' stdout or stderr.
        '''

        self._s = stream
        self._q = Queue()

        def _populateQueue(stream, queue):
            '''
            Collect lines from 'stream' and put them in 'queue'.
            '''

            while True:
                if self._t.stopped():
                    return 0

                line = stream.readline()
                if line:
                    queue.put(line)
                else:
                    raise UnexpectedEndOfStream

        self._t = StoppableThread(target = _populateQueue,
                args = (self._s, self._q))
        self._t.daemon = True
        self._t.start() #start collecting lines from the stream

    def readline(self, timeout = None):
        try:
            return self._q.get(block = timeout is not None,
                    timeout = timeout)
        except Empty:
            return None

    def close(self):
        self._t.stop()
        self._t.join(2)
