# Simple ANSI color parser
#
# see http://man7.org/linux/man-pages/man4/console_codes.4.html
#
class AnsiParser:
    def __init__(self):
        # Escape-sequence state machine
        #
        self.state = 0
        self.buffer = ""

        # ANSI states
        #
        self.bold = False
        self.underline = False
        self.blink = False
        self.reverse = False
        self.foreground = 7
        self.background = 0

    def _parse_sgr_code(self, n):
        # select graphic rendition
        #
        if n == 0:
            self.foreground = 7
            self.background = 0
            self.bold = False
            self.underline = False
            self.blink = False
            self.reverse = False
        elif n == 1:            self.bold = True
        elif n == 4:            self.underline = True
        elif n == 5:            self.blink = True
        elif n == 7:            self.reverse = True
        elif 30 <= n <= 37:     self.foreground = n - 30
        elif 39:                self.foreground = 7
        elif 40 <= n <= 47:     self.background = n - 40
        elif 49:                self.background = 0

    def _parse_sgr(self, seq):
        for v in seq.split(";"):
            try:
                n = int(v)
                self._parse_sgr_code(n)
            except ValueError:
                pass
        self.attrib_changed()

    def _parse_multi(self, seq):
        if seq.endswith("m"):
            self._parse_sgr(seq[1:-1])

    def _parse_single(self, seq):
        pass

    def _parse_char(self, c):
        if self.state == 0:
            if c == "\x1B":
                # start of escape sequence
                #
                if self.buffer != "":
                    self.print_text(self.buffer)
                    self.buffer = ""
                self.state = 1
            else:
                self.buffer += c

        elif self.state == 1:
            self.buffer += c
            if c == "[":
                # start of multi character sequence
                #
                self.state = 2
            elif 64 <= ord(c) <= 95:
                # single character sequence
                #
                self._parse_single(self.buffer)
                self.buffer = ""
                self.state = 0
            else:
                # invalid sequence
                #
                self.buffer = ""
                self.state = 0

        elif self.state == 2:
            # multi character sequence
            #
            self.buffer += c
            if 64 <= ord(c) <= 126:
                self._parse_multi(self.buffer)
                self.buffer = ""
                self.state = 0

    def print_text(self, text):
        # callback function
        pass

    def attrib_changed(self):
        # callback function
        pass

    def parse(self, text):
        for c in text:
            self._parse_char(c)

        # flush remaining text
        #
        if self.state == 0:
            if self.buffer != "":
                self.print_text(self.buffer)
                self.buffer = ""
