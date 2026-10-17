"""
Dataset Generator for Smart Public Complaint Analyzer
Generates ~1000 realistic complaints for Dehradun city.
"""
import random
import os

random.seed(42)

# ─── Locations (must match the 18-node Dehradun graph) ───
LOCATIONS = [
    (0, "Clock Tower"),
    (1, "Paltan Bazaar"),
    (2, "Rajpur Road"),
    (3, "ISBT Dehradun"),
    (4, "Dehradun Railway Stn"),
    (5, "Race Course"),
    (6, "Dalanwala"),
    (7, "Clement Town"),
    (8, "Prem Nagar"),
    (9, "Sahastradhara"),
    (10, "Ballupur Chowk"),
    (11, "GMS Road"),
    (12, "Nagar Nigam"),
    (13, "Doon Hospital"),
    (14, "Survey Chowk"),
    (15, "Rispana Bridge"),
    (16, "Collectorate"),
    (17, "IT Park Sahastradhara"),
]

# ─── Workers by department ───
DEPT_WORKERS = {
    "Water Supply":       [7, 8],
    "Electricity":        [9, 10],
    "Roads & Transport":  [11, 12],
    "Sanitation":         [13, 14],
    "Public Safety":      [15, 16],
}

STATUSES = ["PENDING", "REVIEWED", "ASSIGNED", "IN_PROGRESS", "COMPLETED"]

# ─── Realistic complaint templates: (title, description, category, severity) ───
# Severity is assigned to match the actual content realistically

WATER_COMPLAINTS = [
    # Critical (8-10)
    ("Major water pipeline burst on main road", "A large diameter water pipeline has burst near the main intersection causing massive flooding on the road. Water is gushing out uncontrollably and the entire street is submerged. Traffic has come to a standstill and nearby shops are getting flooded. Immediate action needed.", 10),
    ("Contaminated water supply causing illness", "Residents of the colony are falling sick after drinking tap water. The water has turned yellowish-brown and has a foul smell. Multiple families have reported diarrhea and vomiting. We suspect sewage mixing with the drinking water supply line.", 10),
    ("Complete water supply failure for 5 days", "Our entire area has not received any water supply for the past five days. Over 200 families are affected and people are forced to buy water tankers at exorbitant prices. The situation is becoming desperate especially for elderly residents.", 9),
    ("Sewage water flooding residential area", "Raw sewage is overflowing from the main drain and flooding the residential colony. The stench is unbearable and children cannot go to school. There is a serious health hazard as contaminated water is entering homes.", 9),
    ("Dangerous sinkhole from water pipe erosion", "A deep sinkhole has appeared on the road due to underground water pipe leakage that has been eroding the soil for months. The hole is about 4 feet deep and poses an extreme danger to pedestrians and vehicles.", 9),
    ("Water tank overflow causing structural damage", "The municipal water tank on the rooftop of the community building has been overflowing continuously for three days. Water is seeping into the walls and ceilings of apartments below causing significant structural damage.", 8),
    ("Flooding in basement due to broken water main", "The water main near our apartment complex has broken and our entire basement parking is flooded with two feet of water. Cars are getting damaged and the electrical systems in the basement are at risk.", 8),
    # Urgent (5-7)
    ("No water supply since yesterday morning", "We have not received water supply since yesterday morning. The overhead tank is completely empty and we have no water for cooking or bathing. Multiple complaints to the water department have gone unanswered.", 7),
    ("Sewage drain completely blocked", "The main sewage drain near our colony entrance is completely blocked and dirty water is accumulating on the street. The situation gets worse when it rains. Please send a team to clear the blockage.", 7),
    ("Water pressure extremely low on upper floors", "Residents on the third and fourth floors of our apartment building are not getting any water due to extremely low pressure. The ground floor gets water but it doesn't reach upper floors. This has been going on for two weeks.", 6),
    ("Leaking water pipeline on roadside", "There is a continuous water leak from the main pipeline running along the road. Water has been flowing down the street for the past week creating a slippery surface. Wastage of water during shortage season is very concerning.", 6),
    ("Irregular water supply timing", "The water supply schedule has been completely erratic for the past month. Sometimes water comes at 3 AM and sometimes not at all. Working families cannot manage with such unpredictable timing.", 5),
    ("Overhead water tank not cleaned for years", "The community overhead water tank has not been cleaned or maintained for at least two years. Green algae is visible inside and the water quality has deteriorated significantly. Residents are worried about health risks.", 6),
    ("Sewage backup in bathroom drains", "During heavy rain the sewage backs up through our bathroom drains. Dirty water with foul smell enters the house through the toilet and bathroom floor drains. This happens every time it rains heavily.", 7),
    ("Broken hand pump in community park", "The public hand pump installed in the community park has been broken for over a month. Many elderly residents and daily wage workers depended on it for drinking water.", 5),
    ("Water meter showing incorrect readings", "My water meter has been giving incorrect readings for the past three months. The bill amount has tripled despite no change in usage. I suspect the meter is faulty and needs replacement.", 5),
    ("Muddy water coming from taps", "For the past four days we have been getting very muddy and brownish water from our taps. It is not suitable for drinking or cooking. We have to buy bottled water which is expensive for our family.", 6),
    ("Drain water stagnating near school", "Drain water is stagnating in a large pool right next to the primary school entrance. Children have to walk through dirty water to enter the school. Mosquito breeding is rampant in the area.", 7),
    ("Water tanker not arriving as scheduled", "The municipal water tanker that is supposed to come every alternate day has not arrived for the past week. Our colony depends entirely on tanker supply as there are no pipeline connections yet.", 6),
    ("Underground water pipe rusted and leaking", "The underground water supply pipe to our colony is very old and rusted. There are multiple small leaks along the pipeline causing water loss and contamination. The pipe needs complete replacement.", 5),
    # Moderate (3-4)
    ("Request for new water connection", "I recently built a new house and need a water connection from the municipal supply. I have submitted the application two months ago but there has been no progress. Please expedite the process.", 3),
    ("Water supply timing needs adjustment", "The current water supply timing of 5 AM to 7 AM is very inconvenient for working professionals who leave early for office. Request to extend the supply timing till 8 AM.", 3),
    ("Dripping tap in public toilet", "The tap in the public toilet near the bus stand has been dripping continuously. While not an emergency it is causing wastage of water. A simple washer replacement should fix it.", 3),
    ("Request for water purifier installation in park", "The drinking water fountain in the public park does not have any purification system. Children and joggers drink directly from it. Request installation of a basic water purifier.", 3),
    ("Slow drainage in monsoon season", "During monsoon the drainage system in our lane gets overwhelmed and water accumulates for hours. It eventually drains but causes inconvenience. The drain capacity needs to be increased.", 4),
    ("Old water pipes need replacement", "The water pipes in our lane are over 30 years old and are made of iron. They have started corroding from inside affecting water quality. Request replacement with PVC pipes.", 4),
    ("Public tap leaking at the junction", "The public water tap at the road junction has a slow leak from the handle. It wastes water throughout the day. A minor repair is needed.", 3),
    ("Water supply duration reduced", "The daily water supply duration has been reduced from 2 hours to just 45 minutes without any prior notice. While we understand constraints this is insufficient for families with children.", 4),
    # Low (1-2)
    ("Suggestion for rainwater harvesting", "I would like to suggest that the municipal corporation promote rainwater harvesting systems in residential areas. This would help with water conservation during dry months.", 2),
    ("Request for water usage awareness campaign", "Many residents waste water by leaving taps running. A public awareness campaign about water conservation would be very beneficial for the community.", 1),
    ("Minor drip from garden hose connection", "The connection point for the garden hose in the public park has a very minor drip. Not urgent but should be fixed during routine maintenance.", 2),
    ("Paint peeling on water tank exterior", "The exterior paint of the community water tank has peeled off significantly. It looks unsightly and the exposed metal may corrode over time. Request repainting.", 2),
]

ELECTRICITY_COMPLAINTS = [
    # Critical (8-10)
    ("Exposed high voltage wire hanging over playground", "A high voltage electrical wire has snapped and is hanging dangerously low over the children's playground. Sparks are visible from the broken end. This is extremely dangerous and children could be electrocuted. Emergency response needed immediately.", 10),
    ("Transformer explosion in residential colony", "The electricity transformer near our colony exploded this morning with a loud bang and caught fire. The entire area is without power and there are sparking cables on the ground. Fire brigade has been called but the electrical department needs to respond urgently.", 10),
    ("Electric pole fell on house during storm", "During last night's storm an electric pole fell on the roof of a house. Live wires are draped across the building and the family is trapped inside. They cannot come out safely. Please send help immediately.", 10),
    ("Child electrocuted by exposed junction box", "A child received a severe electric shock from an exposed junction box near the park. The box has no cover and live wires are visible. The child has been hospitalized. This junction box has been reported before but no action was taken.", 10),
    ("Massive power outage affecting hospital area", "There has been a complete power outage in the area surrounding the government hospital for over 8 hours. The hospital backup generator has limited fuel. Patient lives are at risk including those on ventilators.", 10),
    ("Burning smell from electrical panel in apartment", "There is a strong burning smell coming from the main electrical distribution panel of our apartment building. We can hear crackling sounds and see slight smoke. There is a serious risk of electrical fire.", 9),
    ("Street light pole electrified after rain", "After yesterday's rain the metal street light pole near the bus stop has become electrified. A person who touched it received a shock. The pole is on a busy sidewalk where many people walk. Extremely dangerous.", 9),
    ("Sparking from overhead power lines", "Heavy sparking is occurring from the overhead power lines near the market area. Sparks are falling on the shops below which have flammable goods. A fire could break out any moment.", 9),
    # Urgent (5-7)
    ("Complete power outage for 18 hours", "Our colony has been without electricity for the past 18 hours. The complaint number given to us has not been resolved. People with medical equipment at home are suffering. This is summer and the heat is unbearable.", 7),
    ("Frequent power cuts disrupting work from home", "We are experiencing 8 to 10 power cuts daily each lasting 30 minutes to 2 hours. This is severely disrupting work from home and online classes for children. The situation has persisted for two weeks.", 6),
    ("Street lights not working on entire stretch", "All the street lights on the 2 km stretch from the main road to our colony have been non-functional for over a month. The road is pitch dark at night making it unsafe especially for women and elderly.", 6),
    ("Low voltage causing appliance damage", "The voltage in our area frequently drops below 160V causing damage to electrical appliances. My refrigerator compressor and washing machine motor have burned out due to low voltage. The stabilizer cannot handle such low input.", 6),
    ("Transformer overloaded and humming loudly", "The transformer serving our area is making an unusually loud humming noise and the surface is very hot to touch. It seems severely overloaded as many new constructions have been connected. Risk of failure or fire.", 7),
    ("Power line sagging dangerously low", "The power line on our street has sagged so low that a truck might touch it. It is barely 10 feet above the road surface. The line needs to be tightened or the pole height increased before an accident happens.", 7),
    ("Electricity meter box damaged by rain", "The electricity meter box outside our house has been damaged by rain and the internal components are exposed to water. This is a safety hazard and may cause short circuit. Need a waterproof replacement box.", 5),
    ("Frequent voltage fluctuations damaging electronics", "There are severe voltage fluctuations in our locality especially during evening hours. Several households have lost expensive electronics like televisions and computers. The fluctuations happen multiple times daily.", 6),
    ("Unauthorized electricity connection creating hazard", "Someone has made an illegal electricity connection by tapping into the main overhead line using bare wires. The makeshift connection is hanging loosely and could shock anyone who touches it.", 7),
    ("Electrical pole tilting dangerously", "The wooden electrical pole near our house has tilted significantly due to soil erosion. The pole could fall any day especially during strong winds. Multiple wires pass through this pole serving the entire lane.", 7),
    # Moderate (3-4)
    ("Street light flickering continuously", "The street light outside our house has been flickering on and off throughout the night. It creates an annoying strobe effect and disturbs sleep. Likely needs a new ballast or bulb replacement.", 3),
    ("Request for additional street light installation", "The new residential lane behind the main market has no street lights at all. Request installation of at least 4 to 5 street lights for safety and convenience of residents.", 4),
    ("Electricity bill discrepancy", "My electricity bill for this month is showing consumption of 800 units when our average usage is only 200 units. There seems to be a meter reading error or a faulty meter. Request investigation and correction.", 4),
    ("Old wiring in government quarters needs replacement", "The internal electrical wiring in the government staff quarters is over 40 years old and uses aluminum conductors. The insulation has degraded and there are frequent minor short circuits.", 5),
    ("Street light remains on during daytime", "The street light at the intersection stays on 24 hours a day including bright daylight. This is a waste of electricity. The automatic sensor or timer seems to be faulty.", 3),
    ("Power supply to community hall irregular", "The community hall loses power frequently during events. The connection seems unstable. Request a dedicated stable connection for the community hall which serves hundreds of residents.", 4),
    ("Dim street lights need brighter bulbs", "The street lights in our lane have very dim yellow bulbs that barely illuminate the road. Request replacement with brighter LED lights for better visibility and safety.", 3),
    # Low (1-2)
    ("Suggestion for solar street lights", "Suggest installing solar powered street lights in our area. This would save electricity costs and ensure lights work even during power cuts. Many cities have adopted this successfully.", 2),
    ("Request for smart electricity meters", "Request the department to install smart digital electricity meters that allow real time monitoring of consumption. The old mechanical meters are prone to errors.", 2),
    ("Paint faded on transformer enclosure", "The transformer enclosure near our park has faded paint and looks rusty. While it functions fine the deteriorating appearance suggests lack of maintenance. Request repainting.", 1),
    ("Cosmetic damage to street light pole", "The street light pole near our gate has some cosmetic damage and rust marks. The light works fine but the pole needs repainting to prevent further corrosion.", 2),
]

ROADS_COMPLAINTS = [
    # Critical (8-10)
    ("Massive sinkhole appeared on busy road", "A large sinkhole approximately 6 feet wide and 8 feet deep has suddenly appeared in the middle of the busy main road. Two wheelers have already fallen into it. This is an extreme danger to all commuters. The area needs immediate barricading and repair.", 10),
    ("Bridge railing collapsed on river crossing", "The metal safety railing on the bridge has collapsed on one side leaving a dangerous gap. Vehicles and pedestrians can fall directly into the river below. Multiple accidents have already been narrowly avoided.", 10),
    ("Major road cave-in after heavy rainfall", "A section of the road approximately 20 meters long has caved in after last night's heavy rainfall. The collapse extends across the full width of the road. Underground sewage pipes are visible. Vehicles cannot pass.", 9),
    ("Road collapsed near school zone", "The road right in front of the school gate has developed a major collapse. Large cracks have appeared and a section has sunk about 2 feet. Hundreds of children use this road daily. Extremely dangerous situation.", 9),
    ("Accident-prone unmarked sharp curve", "There have been 5 accidents at the sharp blind curve on this road in the past month alone. There are no warning signs no speed breakers no convex mirror and no road markings. Last week a motorcyclist died. Urgent safety measures needed.", 9),
    ("Fallen tree blocking entire highway", "A large tree has fallen across the national highway completely blocking both lanes. Traffic is backed up for kilometers. The tree has also brought down power lines. Heavy equipment needed to clear it.", 8),
    ("Manhole cover missing on main road", "A manhole on the busy main road has no cover. It is a gaping hole right in the traffic lane. At night it is invisible to drivers. A car already damaged its wheel falling into it. Someone could die.", 8),
    # Urgent (5-7)
    ("Multiple deep potholes on main arterial road", "The main arterial road connecting to the highway has developed numerous deep potholes. Some are over a foot deep and filled with water making them invisible. Several vehicles have been damaged and a biker was injured last week.", 7),
    ("Traffic signal not working at busy intersection", "The traffic signal at the major intersection has been non-functional for three days. The intersection sees thousands of vehicles daily and without the signal there is constant traffic chaos and near-miss accidents.", 7),
    ("Road completely waterlogged after light rain", "The road in our area gets completely waterlogged even after 30 minutes of light rain. The drainage is blocked and water accumulates to knee height. Vehicles stall and pedestrians cannot cross.", 6),
    ("Speed breakers too high causing vehicle damage", "The speed breakers installed on our road are excessively high and have no markings or paint. They cause severe jolts and have damaged the undercarriage of multiple vehicles. Even ambulances have difficulty crossing.", 5),
    ("Divider broken creating accident risk", "The central road divider has been broken by a truck accident two weeks ago. The broken concrete pieces are scattered on the road and vehicles are crossing over into oncoming traffic through the gap.", 7),
    ("Footpath encroached by vendors", "The entire footpath along the market road has been encroached by street vendors. Pedestrians including elderly and disabled persons are forced to walk on the busy road risking their lives.", 5),
    ("Road markings completely faded", "The lane markings and zebra crossings on the main road have completely faded and are invisible. Drivers are not following lanes leading to chaotic driving and frequent near-miss incidents.", 5),
    ("Construction debris blocking half the road", "A construction site has dumped debris and building materials on the road blocking one entire lane. This has been there for over two weeks causing severe traffic congestion and accidents.", 6),
    ("Flyover approach road has dangerous cracks", "The approach road to the flyover has developed long deep cracks running along its length. The cracks are widening and the road surface is uneven. Heavy vehicles using this road daily are making it worse.", 7),
    ("Missing signboard at dangerous intersection", "The signboard indicating a one-way road at the intersection has been missing for months. Vehicles regularly enter from the wrong side causing head-on near misses. An accident is waiting to happen.", 6),
    ("Road surface melting in extreme heat", "The road surface in our area has become soft and sticky due to extreme heat. Vehicle tires are leaving deep impressions and the bitumen is sticking to shoes of pedestrians. The road was poorly constructed.", 5),
    ("Stray cattle causing accidents on highway", "Large numbers of stray cattle sit in the middle of the highway especially at night. Multiple accidents have occurred including a fatal one last month. Cattle need to be removed and the area needs reflectors.", 7),
    # Moderate (3-4)
    ("Potholes on residential lane", "Our residential lane has developed about a dozen small potholes that make driving difficult. Two-wheelers especially have trouble navigating them. The potholes have been there for about two months.", 4),
    ("Request for speed breaker near school", "There is no speed breaker on the road near the school despite heavy traffic. Vehicles speed through this zone especially in the morning when children are arriving. Request installation of speed breakers.", 4),
    ("Sidewalk tiles broken and uneven", "Several tiles on the sidewalk are broken and have become uneven creating tripping hazards. An elderly person fell and injured themselves last week. The broken tiles need replacement.", 4),
    ("Parking encroachment on residential road", "Vehicles from the nearby commercial establishment are being parked on our narrow residential road blocking access. Residents cannot get their own vehicles in and out. Need no parking signs and enforcement.", 4),
    ("Road needs resurfacing after pipeline work", "The road was dug up for pipeline work three months ago and was supposed to be resurfaced. Only a temporary patch was applied which has already broken. The road is in worse condition than before.", 4),
    ("Street name boards missing in new colony", "Our newly developed colony has no street name boards or house number plates. This causes problems for visitors delivery services and most importantly emergency vehicles like ambulances.", 3),
    ("Faded zebra crossing near hospital", "The zebra crossing marks near the hospital entrance have completely faded. Patients and visitors have difficulty crossing the busy road. The crossing needs fresh paint.", 3),
    ("Road humps need repainting", "The speed breakers on our road have lost their yellow and black paint markings. At night drivers cannot see them leading to sudden jolts. Request repainting of all speed breakers.", 3),
    # Low (1-2)
    ("Suggestion for cycling lane on main road", "I would like to suggest creating a dedicated cycling lane on the main road. Many people cycle to work but feel unsafe sharing the road with heavy vehicles.", 2),
    ("Request for road beautification", "The road dividers in our area look plain and unattractive. Suggest planting flowering shrubs and installing decorative lights like other cities have done for beautification.", 1),
    ("Minor crack in sidewalk", "There is a minor crack developing on the sidewalk near the bus stop. Not urgent but should be repaired during routine maintenance to prevent it from worsening.", 2),
    ("Old milestone needs replacement", "The milestone marker on the highway is very old and barely readable. Suggest replacing it with a modern reflective sign for better visibility.", 2),
]

SANITATION_COMPLAINTS = [
    # Critical (8-10)
    ("Toxic waste dumped near water source", "Industrial toxic waste has been illegally dumped near the community borewell which supplies drinking water to over 500 families. The chemical barrels are leaking into the ground and the water has started tasting different. This is a major health emergency.", 10),
    ("Sewage overflowing into residential homes", "The main sewage line has burst and raw sewage is flowing directly into the ground floors of several houses in our lane. The contaminated water is ankle-deep inside homes. Families have been forced to evacuate. Health hazard is extreme.", 10),
    ("Dead animal carcass rotting on main road for days", "A large animal carcass has been lying on the main road for over four days. It has started decomposing and the stench is overwhelming. Flies and stray dogs are feeding on it spreading disease. Despite complaints no one has come to remove it.", 8),
    ("Medical waste dumped in public area", "Used syringes bandages and blood-stained medical waste have been dumped in an open area near the residential colony. Children play in this area. This is a severe biohazard and could spread infections.", 9),
    ("Garbage dump site overflowing onto road", "The designated garbage collection point has not been cleared for over two weeks and has overflowed onto the main road. Rotting garbage is spreading across the street attracting rats and stray dogs. The smell is unbearable.", 8),
    # Urgent (5-7)
    ("Garbage not collected for one week", "The municipal garbage collection vehicle has not visited our lane for the past 7 days. Garbage bags are piling up outside every house attracting flies rats and stray dogs. The decomposing organic waste is creating a terrible stench.", 7),
    ("Open drain emitting toxic fumes", "The open drain running through our locality is emitting extremely foul and possibly toxic fumes especially in the evening hours. Many residents are experiencing headaches and nausea. The drain seems to be carrying industrial effluent.", 7),
    ("Illegal garbage dumping in vacant plot", "People are regularly dumping household and construction garbage in the vacant plot next to our house. The pile has grown very large and is attracting pests. The area has become an eyesore and health hazard.", 6),
    ("Overflowing community dustbin for days", "The large community dustbin at the corner has been overflowing for five days. Garbage is spilling onto the road and into the drain. The stench makes it difficult for nearby residents and shopkeepers.", 6),
    ("Public toilet in terrible unsanitary condition", "The public toilet near the bus stand is in an extremely filthy condition. There is no water supply the toilet bowls are clogged and the floor is covered in waste. People are forced to defecate in the open nearby.", 7),
    ("Stagnant water breeding mosquitoes in large numbers", "A large pool of stagnant drain water has accumulated near the community center and has become a massive mosquito breeding ground. Many residents have fallen ill with dengue and malaria in the past month.", 7),
    ("Construction debris not cleared after building work", "A building was constructed in our neighborhood and the construction debris including broken bricks cement bags and iron rods has been left on the roadside for two months. It blocks pedestrian movement and looks terrible.", 5),
    ("Clogged drain causing water backup during rains", "The main drain in our colony gets completely clogged with garbage during monsoon season. This causes dirty water to back up and flood the streets. Last year the flooding entered several homes.", 6),
    ("Rats infestation in residential colony", "There is a severe rat infestation in our colony due to the overflowing garbage dump nearby. Rats are entering houses at night and have contaminated food supplies. Some children have been bitten. Need pest control.", 7),
    ("Dead fish smell from polluted canal", "The canal passing through our area has become severely polluted and hundreds of dead fish are floating on the surface. The stench of rotting fish is making it difficult to live nearby. The water is dark and oily.", 6),
    # Moderate (3-4)
    ("Need more dustbins in market area", "The market area has very few dustbins and as a result shopkeepers and visitors throw garbage on the road. Request installation of at least 10 additional dustbins at regular intervals.", 3),
    ("Sweeper not coming to our lane regularly", "The municipal sweeper assigned to our lane comes only 2-3 times a week instead of daily. Leaves dust and litter accumulate on the road. Request regular daily sweeping.", 4),
    ("Public urination problem near park wall", "People are regularly urinating against the park boundary wall creating an unbearable smell. Despite painted warnings the problem persists. Request installation of a public urinal nearby.", 4),
    ("Garbage segregation not being followed", "The wet and dry waste segregation system announced by the municipality is not being enforced in our area. Garbage collectors mix everything together making recycling impossible.", 3),
    ("Drain cleaning needed before monsoon", "The drains in our area need cleaning before the monsoon season starts. Last year the clogged drains caused severe waterlogging. Proactive cleaning would prevent the problem.", 4),
    ("Compost pit in park not maintained", "The compost pit in the community park is not being maintained and has started emitting foul smell. Either it should be managed properly or moved to a less frequented area.", 3),
    ("Leaf burning causing air pollution", "Residents and park maintenance staff regularly burn dry leaves in the evening causing heavy smoke in the neighborhood. This aggravates asthma and respiratory problems especially in children.", 4),
    ("Overflowing sewage manhole in lane", "A sewage manhole in our lane keeps overflowing during peak usage hours in the morning and evening. The sewage water spreads on the street. The manhole was last cleaned over six months ago.", 5),
    # Low (1-2)
    ("Suggestion for recycling center in area", "Our area does not have a recycling center nearby. Residents have to travel far to dispose of electronic waste and recyclables. Suggest setting up a local collection center.", 2),
    ("Request for cleanliness awareness campaign", "Many residents still litter in public spaces. A cleanliness awareness campaign with community participation would help maintain a cleaner neighborhood.", 1),
    ("Park bench needs cleaning", "The benches in the public park have become dirty with bird droppings and dust. They need regular cleaning for visitors to be able to use them comfortably.", 2),
    ("Suggestion for waste-to-energy project", "Suggest implementing a small scale waste to energy project for the municipal solid waste. This would reduce landfill burden and generate clean energy.", 1),
]

SAFETY_COMPLAINTS = [
    # Critical (8-10)
    ("Gang attack on commuters near underpass", "A group of armed men attacked and robbed three commuters near the underpass last night. The victims sustained serious injuries and were hospitalized. The gang has been terrorizing the area for weeks. Immediate police intervention and patrolling needed.", 10),
    ("Fire in residential building third floor", "A fire has broken out on the third floor of the residential building due to an electrical short circuit. Smoke is billowing out and families on upper floors are trapped. Fire brigade has been called but people need rescue immediately.", 10),
    ("Building structure cracking and at risk of collapse", "Large visible cracks have appeared on the load-bearing pillars of the old commercial building in the market. The building is occupied by shops on the ground floor and residences above. Engineers who saw it informally said it could collapse during an earthquake.", 9),
    ("Gas leak in residential area", "Strong smell of gas has been detected in the residential area near the gas pipeline route. Multiple residents are complaining of headaches and dizziness. An explosion could occur if there is a spark. Evacuate and investigate immediately.", 10),
    ("Child kidnapping attempt near school", "A suspicious person tried to lure a school child into a van near the school gate yesterday. Alert parents intervened and the person fled. The vehicle number was noted. Police must investigate and increase patrolling near schools.", 9),
    ("Hit and run accident victim lying on road", "A pedestrian has been hit by a speeding vehicle which fled the scene. The injured person is lying on the road bleeding heavily. Bystanders have called ambulance but police needs to arrive and identify the vehicle from CCTV.", 9),
    ("Illegal liquor shop operating near school", "An illegal liquor shop has opened within 50 meters of a primary school. Drunkards create nuisance during school hours and children feel unsafe. This clearly violates the law. Immediate closure and legal action required.", 8),
    ("Stray dog pack attacking pedestrians", "A pack of about 15 aggressive stray dogs has been attacking pedestrians in the evening hours. Three people have been bitten in the past week including a child who needed multiple rabies shots. The dogs are very territorial and aggressive.", 8),
    # Urgent (5-7)
    ("No police patrolling in our area at night", "There has been no visible police patrolling in our residential area at night for months. Petty thefts and break-ins have increased significantly. Residents feel unsafe especially women returning from late shifts.", 6),
    ("CCTV cameras at intersection not working", "The CCTV cameras installed at the major intersection have not been functional for over two months. This area has seen multiple chain snatching incidents. Working cameras would help identify criminals.", 6),
    ("Street harassment of women near bus stop", "Women commuters face regular harassment near the bus stop in the evening hours. Groups of men make lewd comments and follow women. Multiple complaints to police have not resulted in any visible action.", 7),
    ("Suspicious activities in abandoned building", "An abandoned building in our neighborhood is being used for suspicious activities at night. Strange people are seen entering and leaving at odd hours. Loud noises and sometimes screams are heard. Residents are scared.", 7),
    ("Drunk driving on residential roads at night", "Drunk drivers speed through our narrow residential lanes late at night. They have hit parked vehicles multiple times and nearly run over pedestrians. Speed breakers and police checkpoints are needed.", 6),
    ("Illegal firecrackers storage in shop", "A shop in the market is illegally storing large quantities of firecrackers without proper license or safety measures. This is in a densely populated market area. An accidental fire could be catastrophic.", 8),
    ("Boundary wall of school collapsed", "The boundary wall of the government school has partially collapsed creating a gap. Unauthorized persons can now easily enter the school premises. Children's safety is compromised.", 7),
    ("Uncovered open well in public area", "An old open well in the public area has no cover or railing around it. It is about 30 feet deep. Children play nearby and an accident is very likely. The well needs to be covered or properly fenced.", 7),
    ("Drug peddling near college campus", "Drug peddling activities have been noticed near the college campus. Young students are being targeted. Parents are very concerned. Undercover police operation is needed to bust the drug ring.", 7),
    ("Noise pollution from illegal late-night parties", "An establishment in the residential zone is hosting loud parties every weekend till 3 AM violating all noise pollution norms. The bass vibrations can be felt in nearby houses. Elderly and children cannot sleep.", 5),
    # Moderate (3-4)
    ("Stray dogs roaming in large groups", "Stray dogs are roaming in groups during daytime in our colony. While they have not attacked anyone yet they bark aggressively at passersby especially children. Animal control should sterilize and manage them.", 4),
    ("Broken street light creating dark patch", "The broken street light near the ATM has created a dark patch that is being exploited by pickpockets. Two incidents of phone snatching have occurred in the dark area. Fixing the light would improve safety.", 5),
    ("Abandoned vehicle parked for months", "An old car without number plates has been parked on our street for over three months. Residents suspect it might be stolen or could be used for hiding contraband. Police should investigate and remove it.", 4),
    ("Traffic violation near school zone", "Vehicles regularly violate the speed limit and no-honking zone near the school. Auto-rickshaws park illegally blocking the school gate. Traffic police presence is needed during school hours.", 4),
    ("Gambling activities in public park", "Illegal gambling activities have been taking place in the public park during afternoon hours. Groups of men play cards for money and create a nuisance. Families avoid bringing children to the park.", 5),
    ("Footpath occupied by homeless encampment", "A section of the footpath near the bridge has been occupied by a homeless encampment. While sympathetic to their situation the encampment blocks pedestrian traffic and creates sanitation issues. Rehabilitation efforts needed.", 3),
    ("Security guard missing from government office", "The security guard at the government office building has been absent for weeks. The building is accessible to anyone without any security check. Important government records are stored inside.", 4),
    ("Nuisance from loudspeaker at late hours", "A religious establishment is using loudspeakers well past the permitted hours causing disturbance to nearby residents. Request enforcement of noise pollution rules.", 4),
    # Low (1-2)
    ("Request for CCTV installation in colony", "Our residential colony does not have any CCTV cameras. Request the police department to install cameras at entry and exit points for security monitoring.", 2),
    ("Suggestion for neighborhood watch program", "Suggest organizing a neighborhood watch program with cooperation between residents and local police. This would improve community safety and reduce petty crimes.", 1),
    ("Old police help board needs replacement", "The police helpline number board near the park is very old and the numbers are unreadable. It needs to be replaced with a new board showing updated emergency numbers.", 2),
    ("Request for women safety helpline awareness", "Many women in our area are not aware of the women safety helpline number. Request putting up awareness posters and boards in prominent locations.", 1),
    ("Park closing time not enforced", "The public park is supposed to close at 8 PM but people loiter there till late night. This sometimes leads to anti-social activities. Request enforcement of closing time.", 3),
]

ALL_TEMPLATES = {
    "Water Supply": WATER_COMPLAINTS,
    "Electricity": ELECTRICITY_COMPLAINTS,
    "Roads & Transport": ROADS_COMPLAINTS,
    "Sanitation": SANITATION_COMPLAINTS,
    "Public Safety": SAFETY_COMPLAINTS,
}

# ─── Citizen names for user generation ───
FIRST_NAMES = [
    "Aarav", "Vivaan", "Aditya", "Vihaan", "Arjun", "Sai", "Reyansh", "Ayaan", "Krishna", "Ishaan",
    "Ananya", "Diya", "Myra", "Sara", "Aanya", "Aadhya", "Ira", "Riya", "Priya", "Neha",
    "Rohan", "Karan", "Nikhil", "Rahul", "Vikram", "Saurabh", "Manish", "Pankaj", "Deepak", "Sunil",
    "Pooja", "Sneha", "Ankita", "Divya", "Meera", "Kavita", "Shweta", "Rekha", "Sunita", "Geeta",
    "Mohit", "Gaurav", "Abhishek", "Rajat", "Ashish", "Tarun", "Varun", "Kunal", "Sachin", "Neeraj"
]

LAST_NAMES = [
    "Sharma", "Verma", "Gupta", "Singh", "Negi", "Rawat", "Chauhan", "Bisht", "Joshi", "Pant",
    "Thapa", "Nautiyal", "Semwal", "Rana", "Dhasmana", "Barthwal", "Kandpal", "Dimri", "Bhatt", "Gairola",
    "Tiwari", "Mishra", "Dubey", "Pandey", "Srivastava", "Agarwal", "Bansal", "Kapoor", "Malhotra", "Mehra"
]

# ─── Additional description variations to multiply templates ───
LOCATION_DETAILS = [
    "near the main market", "close to the residential colony", "near the school",
    "opposite the hospital", "next to the park", "behind the police station",
    "near the bus stand", "along the highway", "in the inner lane",
    "near the temple", "close to the petrol pump", "near the shopping complex",
    "behind the community center", "adjacent to the playground", "near the cinema hall",
    "opposite the bank", "near the ATM", "close to the pharmacy",
    "near the post office", "behind the library"
]

# Generate dates spanning 2025-2026
def random_date():
    year = random.choice([2025, 2026])
    month = random.randint(1, 12 if year == 2025 else 10)
    day = random.randint(1, 28)
    hour = random.randint(6, 22)
    minute = random.randint(0, 59)
    return f"{year}-{month:02d}-{day:02d} {hour:02d}:{minute:02d}"

def generate_users(start_id, count, output_path):
    """Generate citizen user accounts."""
    users = []
    used_usernames = set()
    for i in range(count):
        uid = start_id + i
        first = random.choice(FIRST_NAMES)
        last = random.choice(LAST_NAMES)
        full_name = f"{first} {last}"
        
        # Generate unique username
        username = f"{first.lower()}_{last.lower()}_{random.randint(1,99)}"
        while username in used_usernames:
            username = f"{first.lower()}_{last.lower()}_{random.randint(1,999)}"
        used_usernames.add(username)
        
        password = f"pass{random.randint(1000,9999)}"
        phone = f"98{random.randint(10000000, 99999999)}"
        email = f"{first.lower()}.{last.lower()}{random.randint(1,99)}@gmail.com"
        role = "USER"
        reg_date = random_date()
        dept = "General"
        loc_id = random.randint(0, 17)
        avail = 1
        
        line = f"{uid}|{username}|{password}|{full_name}|{phone}|{email}|{role}|{reg_date}|{dept}|{loc_id}|{avail}"
        users.append((uid, line))
    
    with open(output_path, "a", encoding="utf-8") as f:
        for _, line in users:
            f.write(line + "\n")
    
    return [u[0] for u in users]

def generate_complaints(count, user_ids, output_path):
    """Generate realistic complaints."""
    complaints = []
    
    categories = list(ALL_TEMPLATES.keys())
    
    for i in range(1, count + 1):
        # Pick a random category and template
        category = random.choice(categories)
        templates = ALL_TEMPLATES[category]
        title, desc, severity = random.choice(templates)
        
        # Add location-specific detail to make each entry unique
        loc_detail = random.choice(LOCATION_DETAILS)
        desc_variations = [
            f"{desc} This is {loc_detail}.",
            f"{desc} The exact location is {loc_detail}.",
            f"{desc} Incident reported {loc_detail}.",
            f"{desc} Situation observed {loc_detail}.",
            f"{desc} Problem area is {loc_detail}.",
        ]
        final_desc = random.choice(desc_variations)
        
        # Slight severity variation (±1) to add realism
        sev_var = random.choice([-1, 0, 0, 0, 1])
        final_sev = max(1, min(10, severity + sev_var))
        
        # Pick location
        loc_id, loc_name = random.choice(LOCATIONS)
        
        # Pick user
        user_id = random.choice(user_ids)
        
        # Assign department same as category
        department = category
        
        # Determine status distribution:
        # ~25% PENDING, ~15% REVIEWED, ~20% ASSIGNED, ~15% IN_PROGRESS, ~25% COMPLETED
        status_roll = random.random()
        if status_roll < 0.25:
            status = "PENDING"
            worker_id = -1
            completed_date = "N/A"
        elif status_roll < 0.40:
            status = "REVIEWED"
            worker_id = -1
            completed_date = "N/A"
        elif status_roll < 0.60:
            status = "ASSIGNED"
            worker_id = random.choice(DEPT_WORKERS[category])
            completed_date = "N/A"
        elif status_roll < 0.75:
            status = "IN_PROGRESS"
            worker_id = random.choice(DEPT_WORKERS[category])
            completed_date = "N/A"
        else:
            status = "COMPLETED"
            worker_id = random.choice(DEPT_WORKERS[category])
            completed_date = random_date()
        
        # Duplicate: ~5% flagged as duplicates
        is_duplicate = 1 if random.random() < 0.05 and i > 10 else 0
        duplicate_of = random.randint(1, i - 1) if is_duplicate and i > 1 else -1
        
        # Irrelevant: ~2%
        is_irrelevant = 1 if random.random() < 0.02 else 0
        
        submitted_date = random_date()
        
        # NLP score close to severity
        nlp_score = max(1, min(10, final_sev + random.choice([-1, 0, 0, 1])))
        
        line = (f"{i}|{user_id}|{title}|{final_desc}|{category}|{loc_name}|{loc_id}|"
                f"{final_sev}|{nlp_score}|{is_duplicate}|{duplicate_of}|{is_irrelevant}|"
                f"{status}|{department}|{worker_id}|{submitted_date}|{completed_date}")
        
        complaints.append(line)
    
    with open(output_path, "w", encoding="utf-8") as f:
        for line in complaints:
            f.write(line + "\n")
    
    return len(complaints)

# ─── Main ───
if __name__ == "__main__":
    # Detect data directory (app runs from bin/)
    data_dirs = ["data", "bin/data"]
    data_dir = None
    for d in data_dirs:
        if os.path.isdir(d):
            data_dir = d
            break
    if data_dir is None:
        data_dir = "data"
        os.makedirs(data_dir, exist_ok=True)
    
    users_path = os.path.join(data_dir, "users.txt")
    complaints_path = os.path.join(data_dir, "complaints.txt")
    
    print(f"Data directory: {data_dir}")
    print(f"Generating 50 citizen user accounts...")
    user_ids = generate_users(17, 50, users_path)  # Start from ID 17 (after seeded accounts)
    print(f"  Created {len(user_ids)} users (IDs {user_ids[0]}-{user_ids[-1]})")
    
    print(f"Generating 1000 complaints...")
    count = generate_complaints(1000, user_ids, complaints_path)
    print(f"  Created {count} complaints in {complaints_path}")
    
    # Stats
    cats = {}
    sevs = {i: 0 for i in range(1, 11)}
    stats = {}
    with open(complaints_path, "r", encoding="utf-8") as f:
        for line in f:
            parts = line.strip().split("|")
            if len(parts) >= 17:
                cat = parts[4]
                sev = int(parts[7])
                st = parts[12]
                cats[cat] = cats.get(cat, 0) + 1
                sevs[sev] = sevs.get(sev, 0) + 1
                stats[st] = stats.get(st, 0) + 1
    
    print(f"\n  Category Distribution:")
    for cat, cnt in sorted(cats.items()):
        print(f"    {cat:20s}: {cnt}")
    
    print(f"\n  Severity Distribution:")
    for sev in range(1, 11):
        bar = "█" * (sevs[sev] // 5)
        print(f"    Severity {sev:2d}: {sevs[sev]:4d} {bar}")
    
    print(f"\n  Status Distribution:")
    for st, cnt in sorted(stats.items()):
        print(f"    {st:15s}: {cnt}")
    
    print(f"\nDone! Dataset generated successfully.")
