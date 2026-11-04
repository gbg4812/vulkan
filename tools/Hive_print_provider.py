import lldb

lldb.formatters.Logger._lldb_formatters_debug_level = 1


class HiveChildProvider:
    def __init__(self, valobj: lldb.SBValue, internal_dict):
        """
        This call should initialize the Python object using valobj as the
        variable to provide synthetic children for.
        """
        logger = lldb.formatters.Logger.Logger()
        logger >> "Initializing Hive provider"
        self.valobj = valobj

    def num_children(self, max_children: int) -> int:
        """
        This call should return the number of children that you want your
        object to have[1].
        """
        logger = lldb.formatters.Logger.Logger()
        num = self.valobj.GetChildMemberWithName("_size").GetValueAsUnsigned(0)
        logger >> "Geting number of childs " + str(num)
        return num

    def get_child_index(self, name: str) -> int:
        """
        This call should return the index of the synthetic child whose name is
        given as the argument. Array subscripting, names in the form "[N]", is
        automatically supported.
        Return -1 if there is no child at the index.
        """
        try:
            return int(name.lstrip("[").rstrip("]"))
        except:
            return -1

    def get_child_at_index(self, index: int) -> lldb.SBValue | None:
        """ "
        This call should return a new LLDB SBValue object representing the
        child at the index given as argument.
        """
        if index >= 0 and self.num_children(0) > index:
            logger = lldb.formatters.Logger.Logger()
            logger >> "Getting child: " + str(index)

            bk = index // self.chunk_size
            logger >> "block: " + str(bk)
            bk_index = index % self.chunk_size
            logger >> "index: " + str(bk_index)
            # TODO: fix
            logger >> self.bk_vec.GetType().GetName()
            bk_start = self.bk_vec.GetChildAtIndex(bk)
            logger >> bk_start.GetType().GetName()
            child = bk_start.CreateChildAtOffset(
                "[" + str(index) + "]", bk_index * self.elem_size, self.elem_type
            )
            logger >> child.GetType().GetName()
            return child
        return None

    def update(self) -> bool:
        """ "
        This call should be used to update the internal state of this Python
        object whenever the state of the variables in LLDB changes.[2]
        Also, this method is invoked before any other method in the interface.
        """

        logger = lldb.formatters.Logger.Logger()
        logger >> "Updating Hive Provider"
        self.bk_vec = self.valobj.GetChildMemberWithName("_blocks")
        self.bk_vec.SetPreferSyntheticValue(True)
        self.elem_type = self.valobj.GetType().GetTemplateArgumentType(0)
        self.chunk_size = self.valobj.GetChildMemberWithName(
            "_chunk_size"
        ).GetValueAsUnsigned(0)

        self.elem_size = self.elem_type.GetByteSize()
        return False

    def has_children(self) -> bool:
        """
        This call should return True if this object might have children, and
        False if this object can be guaranteed not to have children.[3]
        """
        return True
