from . import soducocxx as __soducocxx
from .Parser2 import Parser
import pathlib

# FIXME: use ressources
street_names = pathlib.Path(__file__).parent.parent / "resources/denominations-emprises-voies-actuelles.csv"


import cProfile

class Application(__soducocxx.Application):
    parser = Parser(street_names)

    def __init__(self, uri: str, page: int, progress = None, deskew_only=False):
        super().__init__(uri, page, progress, deskew_only)


    def GetDocument(self):
        self.__doc = super().GetDocument()

        #pr = cProfile.Profile()
        #pr.enable()
        for x in self.__doc:
            if x["type"] == "ENTRY":
                text = x.get("text");
                x.update(self.parser.decode(text))
                del x["text"]
            if x["type"] == "ENTRY" or x["type"] == "TITLE_LEVEL_1" or x["type"] == "TITLE_LEVEL_2":
                if x.get("origin") is None:
                    x["origin"] = "computer"
                if x.get("checked") is None:
                    x["checked"] = False
        #pr.disable()
        #pr.print_stats()
        return self.__doc

