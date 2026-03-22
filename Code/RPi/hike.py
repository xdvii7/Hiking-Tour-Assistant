MET_HIKING = 6
KCAL_PER_STEP = 0.04

class HikeSession:
    id = 0
    km = 0
    steps = 0
    kcal = -1
    duration = 0 
    coords = []

    # represents a computationally intensive calculation done by lazy execution.
    def calc_kcal(self):
        self.kcal = MET_HIKING * KCAL_PER_STEP * self.steps

    def durationFormatted(self):
        h = self.duration // 3600
        m = (self.duration % 3600) // 60
        s = self.duration % 60
        return f"{h:02}:{m:02}:{s:02}"

    def __repr__(self):
        return f"HikeSession{{{self.id}, {self.km}(km), {self.steps}(steps), {self.kcal:.2f}(kcal), {self.duration} (seconds)}}"   

def to_list(s: HikeSession) -> list:
    return [s.id, s.km, s.steps, s.kcal, s.duration]

def from_list(l: list) -> HikeSession:
    s = HikeSession()
    s.id = l[0]
    s.km = l[1]
    s.steps = l[2]
    s.kcal = l[3]
    s.duration = int(l[4])
    return s
