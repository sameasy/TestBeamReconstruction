import argparse

#custom action to set multiple flags when calling '--all'
class AllAction(argparse.Action):
    def __init__(self, option_strings, dest, vars_to_ignore, **kwargs):
        self.vars_to_ignore = vars_to_ignore
        super(AllAction, self).__init__(option_strings, dest, nargs=0, **kwargs)
         
    def __call__(self, parser, namespace, values, option_string=None):
        for k,v in namespace.__dict__.items():
            if k not in self.vars_to_ignore:
                namespace.__dict__[k] = True
        
def add_args(parser, mode):
    def resp_res():
        parser.add_argument(
            '--analyze',
            action='store_true',
            help='Analyze the data.'
        )
        parser.add_argument(
            '--plot',
            action='store_true',
            help='Plot the data.'
        )
        
        requiredNamedGroup = parser.add_argument_group('required named arguments')
        requiredNamedGroup.add_argument(
            '--datatype',
            type=str,
            choices=['data', 'sim_proton', 'sim_noproton'],
            required=True,
            help='Choose the datatype to run the analysis on: "data" OR "sim_proton" OR "sim_noproton"'
        )
        return parser.parse_known_args()

    def clusters():
        parser.add_argument(
            '--hits',
            action='store_true',
            help='Run the cluster analysis on the number of hits per cluster..'
        )
        parser.add_argument(
            '--energies',
            action='store_true',
            help='Run the cluster analysis on the energy per cluster.'
        )
        parser.add_argument(
            '--numbers',
            action='store_true',
            help='Run the cluster analysis on the number of clusters.'
        )
        parser.add_argument(
            '--posx',
            action='store_true',
            help='Run the cluster analysis on the X position of clusters.'
        )
        parser.add_argument(
            '--posy',
            action='store_true',
            help='Run the cluster analysis on the Y position of clusters.'
        )
        parser.add_argument(
            '--posx_posy',
            action='store_true',
            help="Run the cluster analysis on the X and Y positions f clusters"
        )


        variables_to_ignore = ['datatype']
        parser.add_argument(
            '--all',
            action=AllAction,
            vars_to_ignore=variables_to_ignore,
            help='Run the full cluster nalysis'
        )
        
        requiredNamedGroup = parser.add_argument_group('required named arguments')
        requiredNamedGroup.add_argument(
            '--datatype',
            type=str,
            choices=['data', 'sim_proton', 'sim_noproton'],
            required=True,
            help='Choose the datatype to run the analysis on: "data" OR "sim_proton" OR "sim_noproton"'
        )

        return parser.parse_known_args()

    def layers():
        parser.add_argument(
            '--densities',
            action='store_true',
            help='Run the layer analysis on the CLUE densities'
        )
        parser.add_argument(
            '--distances',
            action='store_true',
            help='Run the layer analysis on the CLUE distances'
        )
        parser.add_argument(
            '--densities_distances',
            action='store_true',
            help='Run the layer analysis on the CLUE densities and distances together in the same plot'
        )
        parser.add_argument(
            '--densities_2D',
            action='store_true',
            help='Run the layer analysis on the CLUE densities, having all the layer information on the same plot'
        )
        parser.add_argument(
            '--distances_2D',
            action='store_true',
            help='Run the layer analysis on the CLUE distances, having all the layer information on the same plot'
        )
        parser.add_argument(
            '--hits_fraction',
            action='store_true',
            help='Run the layer analysis on the fraction of clusterized hits'
        )
        parser.add_argument(
            '--energy_fraction',
            action='store_true',
            help='Run the layer analysis on the fraction of clusterized energy'
        )
        parser.add_argument(
            '--posx_posy',
            action='store_true',
            help="Run the layer analysis on the hits' X and Y positions in the same plot"
        )
        
        variables_to_ignore = ['datatype', 'distances_2D', 'densities_2D', 'posx_posy']
        parser.add_argument(
            '--all',
            action=AllAction,
            vars_to_ignore=variables_to_ignore,
            help='Run the full layer analysis'
        )
        
        requiredNamedGroup = parser.add_argument_group('required named arguments')
        requiredNamedGroup.add_argument(
            '--'+variables_to_ignore[0],
            type=str,
            choices=['data', 'sim_proton', 'sim_noproton'],
            required=True,
            help='Choose the datatype to run the analysis on: "data" OR "sim_proton" OR "sim_noproton"'
        )
        return parser.parse_known_args()

    if mode == 'resp_res':
        return resp_res()
    elif mode == 'clusters':
        return clusters()
    elif mode == 'layers':
        return layers()
    else:
        raise ValueError('AddArgs: The mode provided is not supported.')